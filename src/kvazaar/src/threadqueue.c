/*****************************************************************************
 * This file is part of Kvazaar HEVC encoder.
 *
 * Copyright (C) 2013-2015 Tampere University of Technology and others (see
 * COPYING file).
 *
 * Kvazaar is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * Kvazaar is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Kvazaar.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/

#include "threadqueue.h"

#include <errno.h> // ETIMEDOUT
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "threads.h"


/**
 * \file
 *
 * Lock acquisition order:
 *
 * 1. When locking a job and its dependency, the dependecy must be locked
 * first and then the job depending on it.
 *
 * 2. When locking a job and the thread queue, the thread queue must be
 * locked first and then the job.
 *
 * 3. When accessing threadqueue_job_t.next, the thread queue must be
 * locked.
 */

#define THREADQUEUE_LIST_REALLOC_SIZE 32

#define PTHREAD_COND_SIGNAL(c) \
  if (pthread_cond_signal((c)) != 0) { \
    fprintf(stderr, "pthread_cond_signal(%s=%p) failed!\n", #c, c); \
    assert(0); \
    return 0; \
  }

#define PTHREAD_COND_BROADCAST(c) \
  if (pthread_cond_broadcast((c)) != 0) { \
    fprintf(stderr, "pthread_cond_broadcast(%s=%p) failed!\n", #c, c); \
    assert(0); \
    return 0; \
  }

#define PTHREAD_COND_WAIT(c,l) \
  if (pthread_cond_wait((c),(l)) != 0) { \
    fprintf(stderr, "pthread_cond_wait(%s=%p, %s=%p) failed!\n", #c, c, #l, l); \
    assert(0); \
    return 0; \
  }

#define PTHREAD_LOCK(l) \
  if (pthread_mutex_lock((l)) != 0) { \
    fprintf(stderr, "pthread_mutex_lock(%s) failed!\n", #l); \
    assert(0); \
    return 0; \
  }

#define PTHREAD_UNLOCK(l) \
  if (pthread_mutex_unlock((l)) != 0) { \
    fprintf(stderr, "pthread_mutex_unlock(%s) failed!\n", #l); \
    assert(0); \
    return 0; \
  }


typedef enum {
  /**
   * \brief Job has been submitted, but is not allowed to run yet.
   */
  THREADQUEUE_JOB_STATE_PAUSED,

  /**
   * \brief Job is waiting for dependencies.
   */
  THREADQUEUE_JOB_STATE_WAITING,

  /**
   * \brief Job is ready to run.
   */
  THREADQUEUE_JOB_STATE_READY,

  /**
   * \brief Job is running.
   */
  THREADQUEUE_JOB_STATE_RUNNING,

  /**
   * \brief Job is completed.
   */
  THREADQUEUE_JOB_STATE_DONE,

} threadqueue_job_state;


struct threadqueue_job_t {
  pthread_mutex_t lock;

  threadqueue_job_state state;

  /**
   * \brief Number of dependencies that have not been completed yet.
   */
  int ndepends;

  /**
   * \brief Reverse dependencies.
   *
   * Array of pointers to jobs that depend on this one. They have to exist
   * when the thread finishes, because they cannot be run before.
   */
  struct threadqueue_job_t **rdepends;

  /**
   * \brief Number of elements in rdepends.
   */
  int rdepends_count;

  /**
   * \brief Allocated size of rdepends.
   */
  int rdepends_size;

  /**
   * \brief Reference count
   */
  int refcount;

  /**
   * \brief Pointer to the function to execute.
   */
  void (*fptr)(void *arg);

  /**
   * \brief Argument for fptr.
   */
  void *arg;

  /**
   * \brief Pointer to the next job in the queue.
   */
  struct threadqueue_job_t *next;

};


struct threadqueue_queue_t {
  pthread_mutex_t lock;

  /**
   * \brief Job available condition variable
   *
   * Signalled when there is a new job to do.
   */
  pthread_cond_t job_available;

  /**
   * \brief Job done condition variable
   *
   * Signalled when a job has been completed.
   */
  pthread_cond_t job_done;

  /**
   * Array containing spawned threads
   */
  pthread_t *threads;

  /**
   * \brief Number of threads spawned
   */
  int thread_count;

  /**
   * \brief Number of threads running
   */
  int thread_running_count;

  /**
   * \brief If true, threads should stop ASAP.
   */
  bool stop;

  /**
   * \brief Pointer to the first ready job
   */
  threadqueue_job_t *first;

  /**
   * \brief Pointer to the last ready job
   */
  threadqueue_job_t *last;
};


/**
 * \brief Add a job to the queue of jobs ready to run.
 *
 * The caller must have locked the thread queue and the job. This function
 * takes the ownership of the job.
 */
static void threadqueue_push_job(threadqueue_queue_t * threadqueue,
                                 threadqueue_job_t *job)
{
  assert(job->ndepends == 0);
  job->state = THREADQUEUE_JOB_STATE_READY;

  if (threadqueue->first == NULL) {
    threadqueue->first = job;
  } else {
    threadqueue->last->next = job;
  }

  threadqueue->last = job;
  job->next = NULL;
}


/**
 * \brief Retrieve a job from the queue of jobs ready to run.
 *
 * The caller must have locked the thread queue. The calling function
 * receives the ownership of the job.
 */
static threadqueue_job_t * threadqueue_pop_job(threadqueue_queue_t * threadqueue)
{
  assert(threadqueue->first != NULL);

  threadqueue_job_t *job = threadqueue->first;
  threadqueue->first = job->next;
  job->next = NULL;

  if (threadqueue->first == NULL) {
    threadqueue->last = NULL;
  }

  return job;
}


/**
 * \brief Function executed by worker threads.
 */
static void* threadqueue_worker(void* threadqueue_opaque)
{
  threadqueue_queue_t * const threadqueue = (threadqueue_queue_t *) threadqueue_opaque;

  PTHREAD_LOCK(&threadqueue->lock);

  for (;;) {
    while (!threadqueue->stop && threadqueue->first == NULL) {
      // Wait until there is something to do in the queue.
      PTHREAD_COND_WAIT(&threadqueue->job_available, &threadqueue->lock);
    }

    if (threadqueue->stop) {
      break;
    }

    // Get a job and remove it from the queue.
    threadqueue_job_t *job = threadqueue_pop_job(threadqueue);

    PTHREAD_LOCK(&job->lock);
    assert(job->state == THREADQUEUE_JOB_STATE_READY);
    job->state = THREADQUEUE_JOB_STATE_RUNNING;
    PTHREAD_UNLOCK(&job->lock);
    PTHREAD_UNLOCK(&threadqueue->lock);

    job->fptr(job->arg);

    PTHREAD_LOCK(&threadqueue->lock);
    PTHREAD_LOCK(&job->lock);
    assert(job->state == THREADQUEUE_JOB_STATE_RUNNING);
    job->state = THREADQUEUE_JOB_STATE_DONE;

    PTHREAD_COND_SIGNAL(&threadqueue->job_done);

    // Go through all the jobs that depend on this one, decreasing their
    // ndepends. Count how many jobs can now start executing so we know how
    // many threads to wake up.
    int num_new_jobs = 0;
    for (int i = 0; i < job->rdepends_count; ++i) {
      threadqueue_job_t * const depjob = job->rdepends[i];
      // The dependency (job) is locked before the job depending on it.
      // This must be the same order as in kvz_threadqueue_job_dep_add.
      PTHREAD_LOCK(&depjob->lock);

      assert(depjob->state == THREADQUEUE_JOB_STATE_WAITING ||
             depjob->state == THREADQUEUE_JOB_STATE_PAUSED);
      assert(depjob->ndepends > 0);
      depjob->ndepends--;

      if (depjob->ndepends == 0 && depjob->state == THREADQUEUE_JOB_STATE_WAITING) {
        // Move the job to ready jobs.
        threadqueue_push_job(threadqueue, kvz_threadqueue_copy_ref(depjob));
        num_new_jobs++;
      }

      // Clear this reference to the job.
      PTHREAD_UNLOCK(&depjob->lock);
      kvz_threadqueue_free_job(&job->rdepends[i]);
    }
    job->rdepends_count = 0;

    PTHREAD_UNLOCK(&job->lock);
    kvz_threadqueue_free_job(&job);

    // The current thread will process one of the new jobs so we wake up
    // one threads less than the the number of new jobs.
    for (int i = 0; i < num_new_jobs - 1; i++) {
      pthread_cond_signal(&threadqueue->job_available);
    }
  }

  threadqueue->thread_running_count--;
  PTHREAD_UNLOCK(&threadqueue->lock);
  return NULL;
}


/**
 * \brief Initialize the queue.
 *
 * \return 1 on success, 0 on failure
 */
threadqueue_queue_t * kvz_threadqueue_init(int thread_count)
{
  threadqueue_queue_t *threadqueue = MALLOC(threadqueue_queue_t, 1);
  if (!threadqueue) {
    goto failed;
  }

  if (pthread_mutex_init(&threadqueue->lock, NULL) != 0) {
    fprintf(stderr, "pthread_mutex_init failed!\n");
    goto failed;
  }

  if (pthread_cond_init(&threadqueue->job_available, NULL) != 0) {
    fprintf(stderr, "pthread_cond_init failed!\n");
    goto failed;
  }

  if (pthread_cond_init(&threadqueue->job_done, NULL) != 0) {
    fprintf(stderr, "pthread_cond_init failed!\n");
    goto failed;
  }

  threadqueue->threads = MALLOC(pthread_t, thread_count);
  if (!threadqueue->threads) {
    fprintf(stderr, "Could not malloc threadqueue->threads!\n");
    goto failed;
  }
  threadqueue->thread_count = 0;
  threadqueue->thread_running_count = 0;

  threadqueue->stop = false;

  threadqueue->first              = NULL;
  threadqueue->last               = NULL;

  // Lock the queue before creating threads, to ensure they all have correct information.
  PTHREAD_LOCK(&threadqueue->lock);
  for (int i = 0; i < thread_count; i++) {
    if (pthread_create(&threadqueue->threads[i], NULL, threadqueue_worker, threadqueue) != 0) {
        fprintf(stderr, "pthread_create failed!\n");
        goto failed;
    }
    threadqueue->thread_count++;
    threadqueue->thread_running_count++;
  }
  PTHREAD_UNLOCK(&threadqueue->lock);

  return threadqueue;

failed:
  kvz_threadqueue_free(threadqueue);
  return NULL;
}


/**
 * \brief Create a job and return a pointer to it.
 *
 * The job is created in a paused state. Function kvz_threadqueue_submit
 * must be called on the job in order to have it run.
 *
 * \return pointer to the job, or NULL on failure
 */
threadqueue_job_t * kvz_threadqueue_job_create(void (*fptr)(void *arg), void *arg)
{
  threadqueue_job_t *job = MALLOC(threadqueue_job_t, 1);
  if (!job) {
    fprintf(stderr, "Could not alloc job!\n");
    return NULL;
  }

  if (pthread_mutex_init(&job->lock, NULL) != 0) {
    fprintf(stderr, "pthread_mutex_init(job) failed!\n");
    return NULL;
  }

  job->state = THREADQUEUE_JOB_STATE_PAUSED;
  job->ndepends       = 0;
  job->rdepends       = NULL;
  job->rdepends_count = 0;
  job->rdepends_size  = 0;
  job->refcount       = 1;
  job->fptr           = fptr;
  job->arg            = arg;

  return job;
}


int kvz_threadqueue_submit(threadqueue_queue_t * const threadqueue, threadqueue_job_t *job)
{
  PTHREAD_LOCK(&threadqueue->lock);
  PTHREAD_LOCK(&job->lock);
  assert(job->state == THREADQUEUE_JOB_STATE_PAUSED);

  if (threadqueue->thread_count == 0) {
    // When not using threads, run the job immediately.
    job->fptr(job->arg);
    job->state = THREADQUEUE_JOB_STATE_DONE;
  } else if (job->ndepends == 0) {
    threadqueue_push_job(threadqueue, kvz_threadqueue_copy_ref(job));
    pthread_cond_signal(&threadqueue->job_available);
  } else {
    job->state = THREADQUEUE_JOB_STATE_WAITING;
  }
  PTHREAD_UNLOCK(&job->lock);
  PTHREAD_UNLOCK(&threadqueue->lock);

  return 1;
}


/**
 * \brief Add a dependency between two jobs.
 *
 * \param job           job that should be executed after dependency
 * \param dependency    job that should be executed before job
 *
 * \return 1 on success, 0 on failure
 *
 */
int kvz_threadqueue_job_dep_add(threadqueue_job_t *job, threadqueue_job_t *dependency)
{
  // Lock the dependency first and then the job depending on it.
  // This must be the same order as in threadqueue_worker.
  PTHREAD_LOCK(&dependency->lock);

  if (dependency->state == THREADQUEUE_JOB_STATE_DONE) {
    // The dependency has been completed already so there is nothing to do.
    PTHREAD_UNLOCK(&dependency->lock);
    return 1;
  }

  PTHREAD_LOCK(&job->lock);
  job->ndepends++;
  PTHREAD_UNLOCK(&job->lock);

  // Add the reverse dependency
  if (dependency->rdepends_count >= dependency->rdepends_size) {
    dependency->rdepends_size += THREADQUEUE_LIST_REALLOC_SIZE;
    size_t bytes = dependency->rdepends_size * sizeof(threadqueue_job_t*);
    dependency->rdepends = realloc(dependency->rdepends, bytes);
  }
  dependency->rdepends[dependency->rdepends_count++] = kvz_threadqueue_copy_ref(job);

  PTHREAD_UNLOCK(&dependency->lock);

  return 1;
}


/**
 * \brief Get a new pointer to a job.
 *
 * Increment reference count and return the job.
 */
threadqueue_job_t *kvz_threadqueue_copy_ref(threadqueue_job_t *job)
{
  // The caller should have had another reference.
  assert(job->refcount > 0);
  KVZ_ATOMIC_INC(&job->refcount);
  return job;
}


/**
 * \brief Free a job.
 *
 * Decrement reference count of the job. If no references exist any more,
 * deallocate associated memory and destroy mutexes.
 *
 * Sets the job pointer to NULL.
 */
void kvz_threadqueue_free_job(threadqueue_job_t **job_ptr)
{
  threadqueue_job_t *job = *job_ptr;
  if (job == NULL) return;
  *job_ptr = NULL;

  int new_refcount = KVZ_ATOMIC_DEC(&job->refcount);
  if (new_refcount > 0) {
    // There are still references so we don't free the data yet.
    return;
  }

  assert(new_refcount == 0);

  for (int i = 0; i < job->rdepends_count; i++) {
    kvz_threadqueue_free_job(&job->rdepends[i]);
  }
  job->rdepends_count = 0;

  FREE_POINTER(job->rdepends);
  pthread_mutex_destroy(&job->lock);
  FREE_POINTER(job);
}


/**
 * \brief Wait for a job to be completed.
 *
 * \return 1 on success, 0 on failure
 */
int kvz_threadqueue_waitfor(threadqueue_queue_t * threadqueue, threadqueue_job_t * job)
{
  PTHREAD_LOCK(&job->lock);
  while (job->state != THREADQUEUE_JOB_STATE_DONE) {
    PTHREAD_COND_WAIT(&threadqueue->job_done, &job->lock);
  }
  PTHREAD_UNLOCK(&job->lock);

  return 1;
}


/**
 * \brief Stop all threads after they finish the current jobs.
 *
 * Block until all threads have stopped.
 *
 * \return 1 on success, 0 on failure
 */
int kvz_threadqueue_stop(threadqueue_queue_t * const threadqueue)
{
  PTHREAD_LOCK(&threadqueue->lock);

  if (threadqueue->stop) {
    // The threadqueue should have stopped already.
    assert(threadqueue->thread_running_count == 0);
    PTHREAD_UNLOCK(&threadqueue->lock);
    return 1;
  }

  // Tell all threads to stop.
  threadqueue->stop = true;
  PTHREAD_COND_BROADCAST(&threadqueue->job_available);
  PTHREAD_UNLOCK(&threadqueue->lock);

  // Wait for them to stop.
  for (int i = 0; i < threadqueue->thread_count; i++) {
    if (pthread_join(threadqueue->threads[i], NULL) != 0) {
      fprintf(stderr, "pthread_join failed!\n");
      return 0;
    }
  }

  return 1;
}


/**
 * \brief Stop all threads and free allocated resources.
 *
 * \return 1 on success, 0 on failure
 */
void kvz_threadqueue_free(threadqueue_queue_t *threadqueue)
{
  if (threadqueue == NULL) return;

  kvz_threadqueue_stop(threadqueue);

  // Free all jobs.
  while (threadqueue->first) {
    threadqueue_job_t *next = threadqueue->first->next;
    kvz_threadqueue_free_job(&threadqueue->first);
    threadqueue->first = next;
  }
  threadqueue->last = NULL;

  FREE_POINTER(threadqueue->threads);
  threadqueue->thread_count = 0;

  if (pthread_mutex_destroy(&threadqueue->lock) != 0) {
    fprintf(stderr, "pthread_mutex_destroy failed!\n");
  }

  if (pthread_cond_destroy(&threadqueue->job_available) != 0) {
    fprintf(stderr, "pthread_cond_destroy failed!\n");
  }

  if (pthread_cond_destroy(&threadqueue->job_done) != 0) {
    fprintf(stderr, "pthread_cond_destroy failed!\n");
  }

  FREE_POINTER(threadqueue);
}

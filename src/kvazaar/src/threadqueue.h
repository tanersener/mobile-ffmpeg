#ifndef THREADQUEUE_H_
#define THREADQUEUE_H_
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

/**
 * \ingroup Threading
 * \file
 * Container for worker tasks.
 */

#include "global.h" // IWYU pragma: keep

#include <pthread.h>

typedef struct threadqueue_job_t threadqueue_job_t;
typedef struct threadqueue_queue_t threadqueue_queue_t;

threadqueue_queue_t * kvz_threadqueue_init(int thread_count);

threadqueue_job_t * kvz_threadqueue_job_create(void (*fptr)(void *arg), void *arg);
int kvz_threadqueue_submit(threadqueue_queue_t * threadqueue, threadqueue_job_t *job);

int kvz_threadqueue_job_dep_add(threadqueue_job_t *job, threadqueue_job_t *dependency);

threadqueue_job_t *kvz_threadqueue_copy_ref(threadqueue_job_t *job);

void kvz_threadqueue_free_job(threadqueue_job_t **job_ptr);

int kvz_threadqueue_waitfor(threadqueue_queue_t * threadqueue, threadqueue_job_t * job);
int kvz_threadqueue_stop(threadqueue_queue_t * threadqueue);
void kvz_threadqueue_free(threadqueue_queue_t * threadqueue);

#endif // THREADQUEUE_H_

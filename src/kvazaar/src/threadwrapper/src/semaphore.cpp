/*
Copyright 2019 Tampere University

Permission to use, copy, modify, and/or distribute this software for
any purpose with or without fee is hereby granted, provided that the
above copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "semaphore.h"
#include <condition_variable>
#include <mutex>


class Semaphore {
public:

    Semaphore(int value):
        val_(value) {
    }

    void post() {
        std::unique_lock<std::mutex> lck(mtx_);
        if (++val_ <= 0) {
            cvar_.notify_one();
        }
    }

    void wait() {
        std::unique_lock<std::mutex> lck(mtx_);
        if (--val_ < 0) {
            cvar_.wait(lck);
        }
    }


private:

    int val_;
    std::condition_variable cvar_;
    std::mutex mtx_;

};  // class Semaphore


int sem_destroy(sem_t* sem) {
    delete static_cast<Semaphore*>(*sem);
    *sem = nullptr;
    return 0;
}

int sem_init(sem_t* sem, int, unsigned int value) {
    *sem = new Semaphore(value);
    return 0;
}

int sem_post(sem_t* sem) {
    static_cast<Semaphore*>(*sem)->post();
    return 0;
}

int sem_wait(sem_t* sem) {
    static_cast<Semaphore*>(*sem)->wait();
    return 0;
}

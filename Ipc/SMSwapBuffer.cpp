/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "SMSwapBuffer.h"

#if defined(__APPLE__)
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <errno.h>

int sem_timedwait(sem_t* sem, const struct timespec* abs_timeout) {
    pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    int result = 0;

    pthread_mutex_lock(&mtx);
    while (sem_trywait(sem) != 0) {
        result = pthread_cond_timedwait(&cond, &mtx, abs_timeout);
        if (result == ETIMEDOUT) break;
    }
    pthread_mutex_unlock(&mtx);

    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mtx);

    return result;
}

#endif
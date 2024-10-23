#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <semaphore.h>
#include <pthread.h>

#define QUEUE_SIZE 128

struct work_t {
    void (*fn)(void *data);
    void *data;
};

class ThreadPool {
    private:
        size_t size;
        pthread_t *tids;

        pthread_mutex_t lock;
        sem_t work_sem;
        sem_t space_sem;

        struct work_t *work_buff;
        size_t in, out;

    public:
        ThreadPool(size_t size);
        ~ThreadPool();

        bool send(struct work_t work);
        struct work_t recv();
        static void* start(void *ctx);
};

#endif
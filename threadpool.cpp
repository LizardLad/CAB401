#include <cstddef>
#include <cstdlib>
#include <stdio.h>

#include <threadpool.hpp>

ThreadPool::ThreadPool(size_t size){
    this->size = size;
    this->in = 0;
    this->out = 0;

    int ret;
    ret = sem_init(&(this->work_sem), 0, 0); //New semaphore, shared mem in one process, initialized to 0
    if(ret != 0) {
        perror("[CRITICAL] Failed to create work semaphore for the threadpool");
        exit(EXIT_FAILURE);
    }

    ret = sem_init(&(this->space_sem), 0, QUEUE_SIZE); //New semaphore, shared mem in one process, initialized to 0
    if(ret != 0) {
        perror("[CRITICAL] Failed to create space semaphore for the threadpool");
        exit(EXIT_FAILURE);
    }

    ret = pthread_mutex_init(&(this->lock), nullptr);
    if(ret != 0) {
        perror("[CRITICAL] Failed to create mutex for the threadpool");
        exit(EXIT_FAILURE);
    }

    this->tids = (pthread_t *)calloc(size, sizeof(pthread_t));
    if(this->tids == nullptr) {
        perror("[CRITICAL] Failed to allocate enough memory for the tids in threadpool");
        exit(EXIT_FAILURE);
    }
    this->work_buff = (struct work_t *)calloc(QUEUE_SIZE, sizeof(struct work_t));
    if(this->work_buff == nullptr) {
        perror("[CRITICAL] Failed to allocate enough memory for the work queue in threadpool");
        exit(EXIT_FAILURE);
    }

    for(size_t i = 0; i < size; i++) {
        int ret = pthread_create(&(tids[i]), nullptr, &ThreadPool::start, (void*)this);
        if(ret != 0) { //Don't bother cleaning up
            perror("[CRITICAL] Failed to spawn as many threads as requested");
            exit(EXIT_FAILURE);
        }
    }
}

ThreadPool::~ThreadPool() {
    int ret;
    struct work_t exit_work = {.fn = nullptr, .data=nullptr};
    for(size_t i = 0; i < this->size; i++) {
        while(!this->send(exit_work));
    }
    for(size_t i = 0; i < this->size; i++) {
        void *ret_val; //Ignore value
        pthread_join(this->tids[i], &ret_val);
    }
    ret = sem_destroy(&(this->work_sem));
    if(ret != 0) {
        perror("[WARNING] Failed to destroy semaphore for the threadpool");
    }
    ret = sem_destroy(&(this->space_sem));
    if(ret != 0) {
        perror("[WARNING] Failed to destroy semaphore for the threadpool");
    }
    free(this->tids);
    free(this->work_buff);

    ret = pthread_mutex_destroy(&(this->lock));
    if(ret != 0) {
        perror("[WARNING] Failed to destroy mutex for the threadpool");
    }
}

bool ThreadPool::send(struct work_t work) {
    sem_wait(&(this->space_sem));

    //Just a push to the queue really
    pthread_mutex_lock(&this->lock);
    size_t next_in = (this->in + 1) % QUEUE_SIZE;
    if(next_in == this->out) { //There is no space to append
        pthread_mutex_unlock(&(this->lock)); //Really shouldn't ever happen because of the semaphore
        sem_post(&(this->space_sem));
        return false;
    }

    this->work_buff[this->in] = work;
    this->in = next_in;

    pthread_mutex_unlock(&this->lock);
    sem_post(&(this->work_sem));
    
    return true;
}

struct work_t ThreadPool::recv() {
    struct work_t work = {.fn = nullptr, .data=nullptr};
    
    sem_wait(&(this->work_sem));
    pthread_mutex_lock(&(this->lock));

    work = this->work_buff[this->out];
    this->out = (this->out + 1) % QUEUE_SIZE;

    pthread_mutex_unlock(&this->lock);
    sem_post(&(this->space_sem));

    return work;
}

void* ThreadPool::start(void *ctx) {
    ThreadPool *pool = (ThreadPool *)ctx;
    while(true) {
        struct work_t work = pool->recv();
        if(work.fn == nullptr) {
            break;
        }
        work.fn(work.data);
    }
    return nullptr;
}
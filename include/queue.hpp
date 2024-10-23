#ifndef QUEUE_HPP
#define QUEUE_HPP

#include <cstdlib>
#include <vector>
#include <stdexcept>

#include <pthread.h>
#include <semaphore.h>

template <typename T>

class Queue { //Thread-safe queue
    private:
        T *queue;
        size_t read_head;
        size_t write_head;

        size_t capacity;

        pthread_mutex_t lock;
        
        sem_t consumer_sem;
        sem_t producer_sem;

        pthread_mutex_t multi_grab_lock;
        bool multi_grab;

    public:
    Queue(size_t capacity);
    Queue(size_t capacity, bool multi_grab);
    ~Queue();

    void push(T item);
    T pop();
    std::vector<T> popn(size_t n);
    
    size_t size();
};

template <typename T>
Queue<T>::Queue(size_t capacity, bool multi_grab) {
    pthread_mutex_init(&this->lock, NULL);
    pthread_mutex_init(&this->multi_grab_lock, NULL);
    sem_init(&this->consumer_sem, 0, 0);
    sem_init(&this->producer_sem, 0, capacity);

    queue = new T[capacity];
    read_head = 0;
    write_head = 0;
    this->capacity = capacity;
    this->multi_grab = multi_grab;
}

template <typename T>
Queue<T>::Queue(size_t capacity) {
    pthread_mutex_init(&this->lock, NULL);
    pthread_mutex_init(&this->multi_grab_lock, NULL);
    sem_init(&this->consumer_sem, 0, 0);
    sem_init(&this->producer_sem, 0, capacity);

    queue = new T[capacity];
    read_head = 0;
    write_head = 0;
    this->capacity = capacity;
    multi_grab = false;
}

template <typename T>
Queue<T>::~Queue() {
    pthread_mutex_destroy(&this->lock);
    pthread_mutex_destroy(&this->multi_grab_lock);
    sem_destroy(&this->consumer_sem);
    sem_destroy(&this->producer_sem);
    delete[] queue;
}

template <typename T>
void Queue<T>::push(T item) { //Blocking
    sem_wait(&this->producer_sem);
    pthread_mutex_lock(&this->lock);
    
    queue[write_head] = item;
    write_head = (write_head + 1) % capacity;
    
    pthread_mutex_unlock(&this->lock);
    sem_post(&this->consumer_sem);
}

template <typename T>
T Queue<T>::pop() { //Blocking
    if(!multi_grab) {
        pthread_mutex_lock(&this->multi_grab_lock);
    }
    
    sem_wait(&this->consumer_sem);
    pthread_mutex_lock(&this->lock);

    T item = queue[read_head];
    read_head = (read_head + 1) % capacity;

    pthread_mutex_unlock(&this->lock);
    sem_post(&this->producer_sem);

    if(!multi_grab) {
        pthread_mutex_unlock(&this->multi_grab_lock);
    }
    
    return item;
}

template <typename T>
size_t Queue<T>::size() {
    //The number of stored items is the distance between the read and write heads.
    //It is 0 if they are equal
    //The write should be on the right of the read head if the queue is not empty
    //Since the buffer is circular the write head could be at 100 and the read head at 0 then size=100
    //If the read head is at 100 and the write head is at  0 then size=0
    return (size_t)((ssize_t)write_head - (ssize_t)read_head + (ssize_t)capacity) % capacity;
}

template <typename T>
std::vector<T> Queue<T>::popn(size_t n) {
    if(!multi_grab) {
        throw std::runtime_error("Multi grab not enabled");
    }
    pthread_mutex_lock(&this->multi_grab_lock);
    std::vector<T> items;
    for(size_t i = 0; i < n; i++) {
        items.push_back(pop());
    }
    pthread_mutex_unlock(&this->multi_grab_lock);
    return items;
}

#endif
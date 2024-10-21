#ifndef QUEUE_HPP
#define QUEUE_HPP

#include <cstdlib>
#include <vector>

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

#endif
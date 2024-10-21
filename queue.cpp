#include <cstdlib>
#include <vector>
#include <queue.hpp>

template <typename T>
Queue<T>::Queue(size_t capacity, bool multi_grab) {
    pthread_mutex_init(&lock, NULL);
    pthread_mutex_init(&multi_grab_lock, NULL);
    sem_init(&consumer_sem, 0, 0);
    sem_init(&producer_sem, 0, capacity);

    queue = new T[capacity];
    read_head = 0;
    write_head = 0;
    this->capacity = capacity;
    this->multi_grab = multi_grab;
}

template <typename T>
Queue<T>::Queue(size_t capacity) {
    pthread_mutex_init(&lock, NULL);
    pthread_mutex_init(&multi_grab_lock, NULL);
    sem_init(&consumer_sem, 0, 0);
    sem_init(&producer_sem, 0, capacity);

    queue = new T[capacity];
    read_head = 0;
    write_head = 0;
    this->capacity = capacity;
    multi_grab = false;
}

template <typename T>
Queue<T>::~Queue() {
    pthread_mutex_destroy(&lock);
    pthread_mutex_destroy(&multi_grab_lock);
    sem_destroy(&consumer_sem);
    sem_destroy(&producer_sem);
    delete[] queue;
}

template <typename T>
void Queue<T>::push(T item) { //Blocking
    sem_wait(&producer_sem);
    pthread_mutex_lock(&lock);
    
    queue[write_head] = item;
    write_head = (write_head + 1) % capacity;
    
    pthread_mutex_unlock(&lock);
    sem_post(&consumer_sem);
}

template <typename T>
T Queue<T>::pop() { //Blocking
    if(!multi_grab) {
        pthread_mutex_lock(&multi_grab_lock);
    }
    
    sem_wait(&consumer_sem);
    pthread_mutex_lock(&lock);

    T item = queue[read_head];
    read_head = (read_head + 1) % capacity;

    pthread_mutex_unlock(&lock);
    sem_post(&producer_sem);

    if(!multi_grab) {
        pthread_mutex_unlock(&multi_grab_lock);
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
    pthread_mutex_lock(&multi_grab_lock);
    if(n > size()) {

    }
    std::vector<T> items;
    for(size_t i = 0; i < n; i++) {
        items.push_back(pop());
    }
    pthread_mutex_unlock(&multi_grab_lock);
    return items;
}
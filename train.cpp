#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#include <cstdlib>
#include <cstddef>
#include <vector>

#include <threadpool.hpp>
#include <tokeniser.hpp>
#include <frequency.hpp>
#include <dataset.hpp>
#include <train.hpp>
#include <vocab.hpp>
#include <queue.hpp>
#include <args.hpp>

enum msg_type {
    DATA,
    DATA_FIN,
};

struct sum_vocab_arg {
    Queue<Frequency *> *queue;
    pthread_mutex_t *lock;
    sem_t *sums_finished;
    size_t *sums_occured;
    uint32_t target_sums;
};

void sum_vocab_worker(void *data) {
    struct sum_vocab_arg *arg_p = (struct sum_vocab_arg *)data;
    Queue<Frequency *> *queue = arg_p->queue;
    

    //Consume queue
    std::vector<Frequency *> frequencies = queue->popn(2);
    Frequency *lhs = frequencies[0];
    Frequency *rhs = frequencies[1];
    lhs->add(*rhs);
    queue->push(lhs);
    delete rhs;
    
    pthread_mutex_lock(arg_p->lock);
    (*arg_p->sums_occured)++;
    if(*arg_p->sums_occured == (size_t)arg_p->target_sums) {
        sem_post(arg_p->sums_finished);
    }
    pthread_mutex_unlock(arg_p->lock);

    free(arg_p);
    return;
}

struct train_msg_t {
    enum msg_type type;
    Data *data;
};

struct train_arg {
    Queue<struct train_msg_t> *queue;
    Queue<Frequency *> *reply;
    VOCAB_DTYPE current_vocab_size;
    VOCAB_DTYPE initial_size_of_vocab;
    std::vector<struct Token> *vocab;
};

void train_vocab_worker(void *data) {
    struct train_arg *arg_p = (struct train_arg* )data;
    
    VOCAB_DTYPE current_vocab_size = arg_p->current_vocab_size;
    Queue<struct train_msg_t> *queue = arg_p->queue;
    Queue<Frequency *> *reply = arg_p->reply;
    Tokeniser tokeniser(arg_p->initial_size_of_vocab, *arg_p->vocab);

    free(arg_p);

    Frequency *frequency_p = new Frequency(current_vocab_size); //Initialise the frequency object with the max size

    while(true) {
        struct train_msg_t msg = queue->pop();

        switch(msg.type) {
            case DATA:
                //tokeniser.inplace_transform(msg.data, current_vocab_size - VOCAB_START - 1);
                tokeniser.count_pairs(msg.data, frequency_p);
                break;
            case DATA_FIN:
                reply->push(frequency_p);
                
                return;
        }
    }
    free(arg_p);
    return;
}

int train(struct command_line_args command_line_args, uint32_t processor_count, ThreadPool *pool) {
    if(command_line_args.vocab_size <= 256) {
        fprintf(stderr, "Vocab count too low\n");
        exit(7);
    }

    //Init the vocab file for status calls
    char *vocab_filename = command_line_args.vocab_path;
    FILE *vocab_file = fopen(vocab_filename, "w+");
    struct vocab_file_header_t header = {.preamble = {'V', 'O', 'C', 'A', 'B'}, .complete=false, .len=0, .desired_len=command_line_args.vocab_size};
    fwrite(&header, sizeof(struct vocab_file_header_t), 1, vocab_file);
    fflush(vocab_file);
    fclose(vocab_file);

    //Initialize the dataset 
    std::vector<char *> filepaths;
    for(size_t i = 0; i < command_line_args.file_count; i++) {
        filepaths.push_back(command_line_args.filepaths[i]);
    }
    DatasetFiles dataset(CHUNK_SIZE, filepaths);

    //Initialize the tokeniser
    Tokeniser tokeniser(VOCAB_START);

    //Training loops

    for(size_t i = 0; i < command_line_args.vocab_size-VOCAB_START; i++) {

        //Transform the underlying data in parallel
        size_t dataset_underlying_data_size = dataset.data.size();
        #pragma omp parallel for
        for(size_t j = 0; j < dataset_underlying_data_size; j++) {
            tokeniser.inplace_transform(&dataset.data[j], i);
        }
        //TODO use the thread pool already implemented with Pthreads and the thread safe Queue implementation
        //FIXME optimisation.

        dataset.prepare_chunks();
        //dataset.shuffle(); //Not performing frequency prediction so no need to shuffle

        //Initialize the Queues
        Queue<struct train_msg_t> comms_queue(QUEUE_SIZE);
        Queue<Frequency *> reply_queue(QUEUE_SIZE, true);

        for(size_t j = 0; j < processor_count; j++) {
            struct train_arg *arg = (struct train_arg *)malloc(sizeof(struct train_arg));
            if(arg == nullptr) {
                exit(150);
            }

            arg->queue = &comms_queue;
            arg->reply = &reply_queue;
            arg->initial_size_of_vocab = VOCAB_START;
            arg->current_vocab_size = VOCAB_START + i;
            arg->vocab = &tokeniser.vocab;

            //
            struct work_t data = {.fn=train_vocab_worker, .data=(void*)arg};
            pool->send(data);
        }

        for(size_t j = 0; j < dataset.size(); j++) {
            Data *data = dataset.yeild(); //FIXME if this goes out of scope before being processed it is a use after free
            struct train_msg_t msg = {.type=DATA, .data=data};
            comms_queue.push(msg);
        }

        for(size_t j = 0; j < processor_count; j++) { //Send the request for the frequency object
            struct train_msg_t msg = {.type=DATA_FIN, .data=nullptr};
            comms_queue.push(msg);
        }

        //Receive them back
        size_t processed = 0;
        size_t target_sums = processor_count-1; //Number of workers that have a frequency object that can be summed together to make the final frequency object

        //Set up the sum workers
        pthread_mutex_t sum_worker_lock;
        pthread_mutex_init(&sum_worker_lock, nullptr);

        sem_t sums_finished;
        sem_init(&sums_finished, 0, 0);

        for(size_t j = 0; j < target_sums; j++) {
            struct sum_vocab_arg *arg = (struct sum_vocab_arg *)malloc(sizeof(struct sum_vocab_arg));
            if(arg == nullptr) {
                exit(150);
            }
            

            arg->lock = &sum_worker_lock;
            arg->queue = &reply_queue;
            arg->sums_finished = &sums_finished;
            arg->sums_occured = &processed;
            arg->target_sums = target_sums;

            struct work_t data = {.fn=sum_vocab_worker, .data=(void*)arg};
            pool->send(data);
        }
        sem_wait(&sums_finished); //Flags when we are done
        printf("Finished token %lu\n", i + VOCAB_START);
        sem_destroy(&sums_finished);
        pthread_mutex_destroy(&sum_worker_lock);
        Frequency *frequencies = reply_queue.pop(); //Pop the final frequency object
        tokeniser.update_vocab(frequencies);
        delete frequencies;

        tokeniser.write_vocab(vocab_filename, command_line_args.vocab_size);
    }
    return 0;
}   
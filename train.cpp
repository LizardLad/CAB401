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

struct sum_vocab_arg {
    Queue<Frequency *> *queue;
    pthread_mutex_t *lock;
    sem_t *sums_finished;
    size_t *sums_occured;
    size_t *target_sums;
};

void sum_vocab_worker(void *data) {
    struct sum_vocab_arg *arg_p = (struct sum_vocab_arg *)data;
    Queue<Frequency *> *queue = arg_p->queue;
    free(arg_p);

    //Consume queue
    std::vector<Frequency *> frequencies = queue->popn(2);
    Frequency *lhs = frequencies[0];
    Frequency *rhs = frequencies[1];
    lhs->add(*rhs);
    queue->push(lhs);
    delete rhs;
    
    pthread_mutex_lock(arg_p->lock);
    (*arg_p->sums_occured)++;
    if(*arg_p->sums_occured == *arg_p->target_sums) {
        sem_post(arg_p->sums_finished);
    }
    pthread_mutex_unlock(arg_p->lock);

    return;
}

struct train_msg_t {
    enum msg_type type;
    union {
        struct {
            Data *data;
        } data;
        struct {
            Frequency *frequency_p;
        } frequency;
    } body;
};

struct train_arg {
    Queue<struct train_msg_t> *queue;
    Queue<struct train_msg_t> *reply;
    VOCAB_DTYPE current_vocab_size;
    VOCAB_DTYPE initial_size_of_vocab;
    std::vector<struct Token> *vocab;
};

void train_vocab_worker(void *data) {
    struct train_arg *arg_p = (struct train_arg* )data;
    
    VOCAB_DTYPE current_vocab_size = arg_p->current_vocab_size;
    Queue<struct train_msg_t> *queue = arg_p->queue;
    Queue<struct train_msg_t> *reply = arg_p->reply;
    Tokeniser tokeniser(arg_p->initial_size_of_vocab, *arg_p->vocab); //Initial size of 256 for the vocab

    free(arg_p);

    Frequency *frequency_p = new Frequency(current_vocab_size); //Initialise the frequency object with the max size

    while(true) {
        sem_wait(&msg_p->sem_worker);
        pthread_mutex_lock(&msg_p->lock);

        switch(msg_p->type) {
            case DATA:
                {
                    //Do something with the data
                    Data *data_p = msg_p->msg.data.data;
                    tokeniser.inplace_transform(data_p);
                    tokeniser.count_pairs(data_p, *frequency_p);
                    tokeniser.update_vocab(*frequency_p);
                    msg_p->type = DATA_FIN;
                } 
                break;
            case FREQUENCY_REQUEST:
                {
                    //Send the frequency object back
                    msg_p->type = FREQUENCY_REPLY;
                    msg_p->msg.frequency.frequency_p = frequency_p;
                }
                return;
        }
        pthread_mutex_unlock(&msg_p->lock);
        sem_post(&msg_p->sem_main);
    }
}

int train(struct command_line_args command_line_args, uint32_t processor_count, ThreadPool pool) {
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
        dataset.prepare_chunks();
        dataset.shuffle();

        //Initialize the Queues
        Queue<struct train_msg_t> comms_queue(QUEUE_SIZE);
        Queue<struct train_msg_t> reply_queue(QUEUE_SIZE);

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
            pool.send(data);
        }

        for(size_t j = 0; j < dataset.size(); j++) {
            Data data = dataset.yeild();
            struct train_msg_t msg = {.type=DATA, .body.data.data=&data};
            comms_queue.push(msg);
        }

        for(size_t j = 0; j < processor_count; j++) { //Send the request for the frequency object
            struct train_msg_t msg = {.type=FREQUENCY_REQUEST};
            comms_queue.push(msg);
        }

        //Receive them back
        size_t processed = 0;
        Queue<Frequency *> reply_queue(QUEUE_SIZE);

        //Set up the sum workers
        for(size_t j = 0; j < processor_count; j++) {
            struct sum_vocab_arg *arg = (struct sum_vocab_arg *)malloc(sizeof(struct sum_vocab_arg));
            if(arg == nullptr) {
                exit(150);
            }

            
            struct work_t data = {.fn=sum_vocab_worker, .data=(void*)arg};
            pool.send(data);
        }


        for(size_t j = 0; j < processor_count; j++) {
            struct train_msg_t msg = reply_queue.pop();
            if(msg.type != FREQUENCY_REPLY) {
                fprintf(stderr, "Received a message that was not a frequency reply\n");
                exit(151);
            }

            Frequency *frequency_p = msg.body.frequency.frequency_p;
            frequencies[j] = frequency_p;
            if(j % 2 == 0) {
                //Sum the frequencies
                processed+=2;
            }
        }

        for()


    }
}   
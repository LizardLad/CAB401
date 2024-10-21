#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <cstdlib>
#include <string>
#include <thread>
#include <unistd.h>

#include <threadpool.hpp>
#include <frequency.hpp>
#include <tokeniser.hpp>
#include <dataset.hpp>
#include <train.hpp>
#include <data.hpp>
#include <args.hpp>

enum msg_type {
    DATA,
    DATA_FIN,
    FREQUENCY_REPLY,
    FREQUENCY_REQUEST,
    EXIT,
};

struct msg_t {
    pthread_mutex_t lock;
    sem_t sem_worker; //Semaphore to signal that the worker is ready to receive a message
    sem_t sem_main; //Semaphore to signal that the main thread is ready to receive a message
    enum msg_type type;    
    union {
        struct {
            Data *data;
        } data;
        struct {
            Frequency* frequency_p;
        } frequency;
    } msg;
};

int main(int argc, char **argv, char **envp) {
    const uint32_t processor_count = std::thread::hardware_concurrency();
    ThreadPool pool(processor_count);

    if(argc < 3) {
        fprintf(stderr, "Incorrect number of arguments provided, expect at least mode and project\n");
        exit(1);
    }

    //Argument 0 is the executable
    //Argument 1 is the mode
    // If Argument 1 is train then
    //   Argument 2 is the project
    //   Argument 3 is the target vocab size
    //   Argument 4 is the output file
    //   Argument 5.. are the filepaths to train on
    // Else if Argument 1 is status
    //   Argument 2 is the project
    struct command_line_args command_line_args;
    if(parse_argv(--argc, ++argv, &command_line_args)) {
        exit(2); //Error already printed in the parse_argv function;
    }

    switch(command_line_args.mode) {
        case MODE_TRAIN:
        {
            train(command_line_args, processor_count);
            
        }
        break;
        case MODE_STATUS:
            break;
        default:
            fprintf(stderr, "Bad mode\n");
            break;
    }

    /*for(int i = 0; i < 128; i++) {
        uint64_t *target = (uint64_t *)malloc(sizeof(uint64_t));
        if(target == NULL) {
            exit(150);
        }
        *target = 1LLU << 32;
    
        struct work_t data = {.fn=sum_vocab_worker, .data=(void*)target};
        pool.send(data);
    }*/

    return 0;
}
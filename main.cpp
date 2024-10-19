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
#include <data.hpp>
#include <args.hpp>

enum msg_type {
    DATA,
    FREQUENCY_REPLY,
    FREQUENCY_REQUEST,
    EXIT,
};

struct msg_t {
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

struct train_arg {
    int p[2]; //Pipe
    VOCAB_DTYPE vocab_size;
};

void train_vocab_worker(void *data) {
    struct train_arg *arg_p = (struct train_arg* )data;
    struct train_arg arg = *arg_p;
    free(arg_p);
    //Use the pipe with select so setup FD_SET
    fd_set read_fds, write_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_SET(arg.p[0], &read_fds);
    FD_SET(arg.p[1], &write_fds);

    Frequency *frequency_p = new Frequency(arg.vocab_size); //Initialise the frequency object with the max size
    Tokeniser tokeniser(256); //Initial size of 256 for the vocab

    printf("Worker started\n");

    while(true) {
        int ret = select(1, &read_fds, NULL, NULL, NULL);
        if(ret == -1) {
            perror("[ERROR] Select failed");
            exit(100);
        }

        if(FD_ISSET(arg.p[0], &read_fds)) {
            struct msg_t msg;
            ssize_t bytes = read(arg.p[0], &msg, sizeof(struct msg_t));
            if(bytes == -1) {
                perror("[ERROR] Failed to read from pipe");
                exit(101);
            }

            switch(msg.type) {
                case DATA:
                    {
                        //Do something with the data
                        Data *data_p = msg.msg.data.data;
                        tokeniser.inplace_transform(data_p);
                        tokeniser.count_pairs(data_p, *frequency_p);
                        tokeniser.update_vocab(*frequency_p);
                    } 
                    break;
                case FREQUENCY_REQUEST:
                    {
                        struct msg_t reply;
                        reply.type = FREQUENCY_REPLY;
                        reply.msg.frequency.frequency_p = frequency_p;
                        ssize_t bytes = write(arg.p[1], &reply, sizeof(struct msg_t));
                        if(bytes == -1) {
                            perror("[ERROR] Failed to write to pipe");
                            exit(1);
                        }
                    }
                    return;
            }
        }
    }
}

struct sum_vocab_arg {
    int out_fd; //Pipe
    Frequency *lhs;
    Frequency *rhs;
};

void sum_vocab_worker(void *data) {
    struct sum_vocab_arg *arg_p = (struct sum_vocab_arg *)data;
    struct sum_vocab_arg arg = *arg_p;
    free(arg_p);

    arg.lhs->add(*arg.rhs);
    delete arg.rhs;
    write(arg.out_fd, arg.lhs, sizeof(Frequency *));
    return;
}

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
            if(command_line_args.vocab_size <= 256) {
                fprintf(stderr, "Vocab count too low\n");
                exit(7);
            }

            std::vector<char *> filepaths;
            for(size_t i = 0; i < command_line_args.file_count; i++) {
                filepaths.push_back(command_line_args.filepaths[i]);
            }
            DatasetFiles dataset(CHUNK_SIZE, filepaths);
            
            //Training loop
            for(size_t i = 0; i < command_line_args.vocab_size-VOCAB_START; i++) {
                dataset.prepare_chunks();
                dataset.shuffle();

                int in_fds[processor_count];
                int out_fds[processor_count];

                for(size_t j = 0; j < processor_count; j++) {
                    struct train_arg *arg = (struct train_arg *)malloc(sizeof(struct train_arg));
                    if(arg == NULL) {
                        exit(150);
                    }
                    arg->vocab_size = command_line_args.vocab_size;

                    int in[2];
                    int out[2];
                    if(pipe(in) == -1 || pipe(out) == -1) {
                        perror("[ERROR] Failed to create pipe");
                        exit(101);
                    }
                    arg->p[0] = out[1];
                    arg->p[1] = in[0];

                    in_fds[j] = in[1];
                    out_fds[j] = out[0];
                    
                    struct work_t data = {.fn=train_vocab_worker, .data=(void*)arg};
                    pool.send(data);
                }

                for(size_t j = 0; j < dataset.size(); j++) {
                    //Start sending data to the workers with the file descriptors?
                }
            }
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
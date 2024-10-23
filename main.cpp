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


int main(int argc, char **argv) {
    const uint32_t processor_count = 20;//std::thread::hardware_concurrency();
    ThreadPool *pool = new ThreadPool(processor_count);

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
            train(command_line_args, processor_count, pool);
            break;
        case MODE_STATUS:
            break;
        default:
            fprintf(stderr, "Bad mode\n");
            break;
    }

    return 0;
}
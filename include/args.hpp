#ifndef ARGS_HPP
#define ARGS_HPP

#include <stdint.h>

enum MODE {
    MODE_STATUS,
    MODE_TRAIN
};

struct command_line_args {
    enum MODE mode;
    char *project;
    uint16_t vocab_size;
    uint8_t file_count;
    char *vocab_path;
    char **filepaths;
};

int parse_argv(int argc, char **argv, struct command_line_args *command_line_args);

#endif
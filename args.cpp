#include <cstdint>
#include <cctype>
#include <cstring>
#include <cstdio>

#include <args.hpp>


int parse_argv(int argc, char **argv, struct command_line_args *command_line_args) {
    char *stat_p = argv[0];
    while(*stat_p) {
        *stat_p = toupper(*stat_p);
        stat_p++;
    } if(strlen(argv[0]) < 5) {
        fprintf(stderr, "Mode not train or status\n");
        return 1;
    }

    
    if(strncmp(argv[0], "TRAIN", 5) == 0) {
        command_line_args->project = argv[1];
        command_line_args->mode = MODE_TRAIN;
    } else if(strncmp(argv[0], "STATUS", 6) == 0) {
        command_line_args->vocab_path = argv[1];
        command_line_args->mode = MODE_STATUS;
        return 0;
    } else {
        fprintf(stderr, "Not TRAIN or STATUS, instead mode requested is %s\n", argv[0]);
        return 2;
    }
    if(argc < 5) {
        fprintf(stderr, "Not enough arguments for train mode\n");
        return 3;
    }

    if(sscanf(argv[2], "%hu", &(command_line_args->vocab_size)) != 1) {
        fprintf(stderr, "Could not parse the vocab count as an unsigned short\n");
        return 4;
    }

    command_line_args->vocab_path = argv[3];

    command_line_args->file_count = argc - 4;
    command_line_args->filepaths = &(argv[4]);
    return 0;
}
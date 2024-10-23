#ifndef VOCAB_HPP
#define VOCAB_HPP

#include <stdint.h>

struct vocab_file_header_t {
    char preamble[5]; //{'V', 'O', 'C', 'A', 'B'}
    bool complete;
    uint16_t len;
    uint16_t desired_len;
};

#endif
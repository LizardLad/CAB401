#ifndef TOKENISER_HPP
#define TOKENISER_HPP

#include <vector>

#include <data.hpp>
#include <config.hpp>

struct vocab_file_header_t {
    char preamble[5]; //{'V', 'O', 'C', 'A', 'B'}
    bool complete;
    uint16_t len;
    uint16_t desired_len;
};

struct Token {
    VOCAB_DTYPE pair[2];
    VOCAB_DTYPE token;
};

class Tokeniser {
    private:
        VOCAB_DTYPE initial_size;
        
    public: 
        std::vector<struct Token> vocab;

        Tokeniser(VOCAB_DTYPE initial_size);
        Tokeniser(VOCAB_DTYPE initial_size, std::vector<struct Token> vocab);
        ~Tokeniser();

        void inplace_transform(Data *data);
        void update_vocab(Frequency frequency);
        void count_pairs(Data *data, Frequency frequency);
};

#endif
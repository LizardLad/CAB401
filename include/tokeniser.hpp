#ifndef TOKENISER_HPP
#define TOKENISER_HPP

#include <vector>

#include <data.hpp>
#include <config.hpp>
#include <frequency.hpp>
#include <vocab.hpp>

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

        void inplace_transform(Data *data, VOCAB_DTYPE previous_runs);
        void update_vocab(Frequency *frequency);
        void count_pairs(Data *data, Frequency *frequency);
        void write_vocab(char *vocab_file, VOCAB_DTYPE desired_len);
};

#endif
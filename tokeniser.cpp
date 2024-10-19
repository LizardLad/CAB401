#include <vector>
#include <unordered_map>
#include <config.hpp>
#include <data.hpp>
#include <stddef.h>
#include <frequency.hpp>

#include <tokeniser.hpp>
#include <stdexcept>

Tokeniser::Tokeniser(VOCAB_DTYPE initial_size = VOCAB_START) {
    this->initial_size = initial_size;
}

Tokeniser::~Tokeniser() {
}

void Tokeniser::inplace_transform(Data *data) {
    for(size_t i = 0; i < vocab.size(); i++) {
        for(size_t j = 0; j < data->size()-1; j++) {
            size_t indxs[2] = {0, 0};

            if(data->operator[](j) != vocab[i].pair[0]) {
                continue;
            }
            indxs[0] = j;

            for(j++; j < data->size(); j++) {
                if(data->operator[](j) == SKIP_TOKEN) {
                    continue;
                }
                if(data->operator[](j) != vocab[i].pair[1]) {
                    break;
                }
                indxs[1] = j;
                break;
            }
            if(indxs[1] == 0) {
                continue;
            }
            data->operator[](indxs[0]) = vocab[i].token;
            data->operator[](indxs[1]) = SKIP_TOKEN;
        }
    }
}

void Tokeniser::count_pairs(Data *data, Frequency frequency) {
    for(int i = 0; i < data->size()-1; i++) {
        if(data->operator[](i) == SKIP_TOKEN) {
            continue;
        }
        VOCAB_DTYPE token = data->operator[](i);
        for(i++; i < data->size(); i++) {
            if(data->operator[](i) == SKIP_TOKEN) {
                continue;
            }
            frequency(token, data->operator[](i))++;
        }
    }
}

void Tokeniser::update_vocab(Frequency frequency) {
    VOCAB_DTYPE max_pair[2];
    frequency.get_max_pair(max_pair);
    if(vocab.size() + (size_t)initial_size > SKIP_TOKEN) {
        throw std::runtime_error("Vocab size is too large to fit in the data type");
    }
    Token token = {{max_pair[0], max_pair[1]}, (VOCAB_DTYPE)(vocab.size() + (size_t)initial_size)};
    vocab.push_back(token);
}

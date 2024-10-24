#include <vector>
#include <unordered_map>
#include <stdexcept>

#include <stddef.h>

#include <data.hpp>
#include <vocab.hpp>
#include <config.hpp>
#include <frequency.hpp>
#include <tokeniser.hpp>


Tokeniser::Tokeniser(VOCAB_DTYPE initial_size = VOCAB_START) {
    this->initial_size = initial_size;
}

Tokeniser::~Tokeniser() {
}

void Tokeniser::inplace_transform(Data *data, VOCAB_DTYPE previous_runs=0) {
    for(size_t i = previous_runs; i < vocab.size(); i++) {
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

        size_t skips = 0;
        for(size_t i = 0; i < data->size(); i++) {  //Make the non skippable tokens contiguous
            if(data->operator[](i) == SKIP_TOKEN) { //This improves branch prediction when counting pairs
                skips++;
                continue;
            }
            data->operator[](i-skips) = data->operator[](i);
        }
    }
}

void Tokeniser::count_pairs(Data *data, Frequency *frequency) {
    for(size_t i = 0; i < data->size()-1; i++) {
        if(data->operator[](i) == SKIP_TOKEN) {
            continue;
        }
        VOCAB_DTYPE token = data->operator[](i);
        for(i++; i < data->size(); i++) {
            if(data->operator[](i) == SKIP_TOKEN) {
                continue;
            }
            //printf("Counting pair %d %d\n", token, data->operator[](i));
            frequency->operator()(token, data->operator[](i))++;
            break;
        }
    }
}

void Tokeniser::update_vocab(Frequency *frequency) {
    VOCAB_DTYPE max_pair[2];
    frequency->get_max_pair(max_pair);
    if(vocab.size() + (size_t)initial_size > SKIP_TOKEN) {
        throw std::runtime_error("Vocab size is too large to fit in the data type");
    }
    Token token = {{max_pair[0], max_pair[1]}, (VOCAB_DTYPE)(vocab.size() + (size_t)initial_size)};
    vocab.push_back(token);
}


Tokeniser::Tokeniser(VOCAB_DTYPE initial_size, std::vector<struct Token> vocab) {
    this->initial_size = initial_size;
    this->vocab = vocab;
}

#define TEMP_NAME_LEN 10

void Tokeniser::write_vocab(char *vocab_file, VOCAB_DTYPE desired_len) {
    char tmp_name[TEMP_NAME_LEN]; tmp_name[TEMP_NAME_LEN-1] = '\0';
    for(size_t i = 0; i < TEMP_NAME_LEN-1; i++) {
        tmp_name[i] = 65 + (rand() % 26);
    }

    FILE *vocab_file_p = fopen(tmp_name, "wb");
    struct vocab_file_header_t header = {.preamble={'V', 'O', 'C', 'A', 'B'}, 
    .complete=(vocab.size()==desired_len), 
    .len = (VOCAB_DTYPE)vocab.size(), 
    .desired_len=desired_len};

    fwrite(&header, sizeof(struct vocab_file_header_t), 1, vocab_file_p);
    for(size_t i = 0; i < vocab.size(); i++) {
        fwrite(&vocab[i], sizeof(struct Token), 1, vocab_file_p);
    }

    //Move the file (atomic)
    fclose(vocab_file_p);
    rename(tmp_name, vocab_file);
}
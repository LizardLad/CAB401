#ifndef DATA_H
#define DATA_H

#include <vector>
#include <cstddef>
#include <config.hpp>
#include <cassert>

enum DataSource {
    SRC_FILE,
    SRC_BLOB,
    SRC_CHUNK
};

class Data
{
    private:
        VOCAB_DTYPE *buff;
        size_t buff_size;

        enum DataSource source;
        size_t max_chunk_size;

    public:
        Data(char *data, size_t size, size_t max_chunk_size);
        Data(char *filename, size_t max_chunk_size); //From file
        Data(VOCAB_DTYPE *data, size_t size, size_t max_chunk_size); //From another data object to create chunks

        ~Data();

        VOCAB_DTYPE& operator[] (size_t idx) {
            assert(idx < this->buff_size);
            return this->buff[idx];
        }

        const VOCAB_DTYPE& operator[] (size_t idx) const {
            assert(idx < this->buff_size);
            return this->buff[idx];
        }

        Data get_chunk(size_t idx);
        size_t chunks();

        size_t size() {
            return buff_size;
        }

        void shrink();
};

#endif
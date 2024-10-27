#ifndef DATA_H
#define DATA_H

#include <vector>
#include <cstddef>
#include <cassert>

#include <config.hpp>

enum DataSource {
    SRC_FILE,
    SRC_BLOB,
    SRC_CHUNK,
    SRC_MOVED
};

class Data
{
    private:
        VOCAB_DTYPE *buff;
        size_t buff_size;

        size_t max_chunk_size;

        Data *parent;

    public:
        enum DataSource source;
        
        Data();
        Data(char *data, size_t size, size_t max_chunk_size);
        Data(char *filename, size_t max_chunk_size); //From file
        Data(VOCAB_DTYPE *data, size_t size, size_t max_chunk_size, Data *parent = nullptr); //From another data object to create chunks

        ~Data();
        Data(Data &&other) noexcept; //Move

        Data(const Data &other); // Copy constructor
        Data& operator=(const Data &other); // Copy assignment operator

        VOCAB_DTYPE& operator[] (size_t idx) {
            return this->buff[idx];
        }

        const VOCAB_DTYPE& operator[] (size_t idx) const {
            return this->buff[idx];
        }

        Data get_chunk(size_t idx);
        size_t chunks();

        size_t size() {
            return buff_size;
        }

        void shrink(size_t new_size);

        void swap(Data &other);
};

#endif
#include <config.hpp>
#include <data.hpp>
#include <fstream>

Data::Data(char *data, size_t size, size_t max_chunk_size) {
    this->buff = (VOCAB_DTYPE *)malloc(sizeof(VOCAB_DTYPE) * size);
    if(this->buff == NULL) {
        throw std::runtime_error("Failed to allocate memory for data");
    }
    this->buff_size = size;

    this->max_chunk_size = max_chunk_size;
    for(size_t i = 0; i < size; i++) {
        this->buff[i] = (VOCAB_DTYPE)data[i];
    }
    this->source = SRC_BLOB;
}

Data::Data(char *filename, size_t max_chunk_size) {
    this->source = SRC_FILE;
    this->max_chunk_size = max_chunk_size;

    //Get file size
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if(!file.is_open()) {
        throw std::runtime_error("Failed to open file");
    }
    this->buff_size = file.tellg();
    this->buff = (VOCAB_DTYPE *)malloc(sizeof(VOCAB_DTYPE) * this->buff_size);
    if(this->buff == NULL) {
        throw std::runtime_error("Failed to allocate memory for data");
    }

    file.seekg(0, std::ios::beg);

    //Read file into buffer
    for(size_t i = 0; i < this->buff_size; i++) {
        this->buff[i] = (VOCAB_DTYPE)file.get();
    }
    file.close(); //Close file
}

Data::~Data() {
    if(this->source == SRC_BLOB) {
        free(this->buff);
    }

    if(this->source == SRC_FILE) {
        free(this->buff);
    }
}

void Data::shrink() {
    size_t skips = 0;
    for(int i = this->buff_size-1; i >= 0; i--) {
        if(this->buff[i] == SKIP_TOKEN) {
            skips++;
            continue;
        }
        this->buff[i-skips] = this->buff[i];
    }
    VOCAB_DTYPE *new_buff = (VOCAB_DTYPE *)realloc(this->buff, sizeof(VOCAB_DTYPE) * (this->buff_size-skips));
    if(new_buff != NULL) {
        this->buff = new_buff;
    } //Doesn't really matter if it fails because the new size is smaller
}

Data::Data(VOCAB_DTYPE *data, size_t size, size_t max_chunk_size) {
    this->max_chunk_size = max_chunk_size;
    this->source = SRC_CHUNK;
    this->buff_size = size;
    this->buff = data;
}

Data Data::get_chunk(size_t idx) {
    if(this->source == SRC_CHUNK) {
        return *this;
    }
    
    size_t start = idx * this->max_chunk_size;
    size_t end = (idx+1) * this->max_chunk_size;
    if(end > this->buff_size) {
        end = this->buff_size;
    }
    return Data(this->buff + start, end - start, this->max_chunk_size);
}

size_t Data::chunks() {
    if(this->source == SRC_CHUNK) {
        return 1;
    }
    size_t floor = this->buff_size / this->max_chunk_size;
    size_t rem = this->buff_size % this->max_chunk_size;
    if(rem > 0) {
        return floor + 1;
    }
    return floor;
}
#include <config.hpp>
#include <data.hpp>
#include <fstream>

Data::Data() {
    this->source = SRC_MOVED;
    this->buff = nullptr;
    this->buff_size = 0;
    this->max_chunk_size = 0;
}

Data::Data(VOCAB_DTYPE *data, size_t size, size_t max_chunk_size) {
    this->max_chunk_size = max_chunk_size;
    this->source = SRC_CHUNK;
    this->buff_size = size;
    this->buff = data;
}

Data::Data(char *data, size_t size, size_t max_chunk_size) {
    this->source = SRC_BLOB;

    this->buff = (VOCAB_DTYPE *)malloc(sizeof(VOCAB_DTYPE) * size);
    if(this->buff == NULL) {
        throw std::runtime_error("Failed to allocate memory for data");
    }
    this->buff_size = size;

    this->max_chunk_size = max_chunk_size;
    for(size_t i = 0; i < size; i++) {
        this->buff[i] = (VOCAB_DTYPE)data[i];
    }
    
}

Data::Data(char *filename, size_t max_chunk_size) {
    this->source = SRC_FILE;

    this->max_chunk_size = max_chunk_size;

    //Get file size
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if(!file.is_open()) {
        exit(200);
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
    if(this->source == SRC_BLOB || this->source == SRC_FILE) {
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



Data Data::get_chunk(size_t idx) {
    if(this->source == SRC_MOVED) {
        throw std::runtime_error("Cannot get chunk from moved data");
    }
    if(this->source == SRC_CHUNK) {
        return std::move(*this);
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

Data::Data(Data &&other) noexcept {
    this->buff = other.buff;
    this->buff_size = other.buff_size;
    this->source = other.source;
    this->max_chunk_size = other.max_chunk_size;

    other.buff = nullptr;
    other.buff_size = 0;
    other.source = SRC_MOVED;
    other.max_chunk_size = 0;
}


Data::Data(const Data &other) {
    this->buff_size = other.buff_size;
    this->max_chunk_size = other.max_chunk_size;
    this->source = other.source;

    if(this->source == SRC_CHUNK) {
        this->buff = other.buff;
        
    } else {
        if(other.buff) {
            this->buff = (VOCAB_DTYPE *)malloc(sizeof(VOCAB_DTYPE) * other.buff_size);
            this->source = SRC_BLOB;
            if(this->buff == NULL) {
                throw std::runtime_error("Failed to allocate memory for data");
            }
            std::copy(other.buff, other.buff + other.buff_size, this->buff);
        } else {
            this->buff = nullptr;
        }
    }
}

// Copy assignment operator
Data& Data::operator=(const Data &other) {
    if(this == &other) {
        return *this;
    }
    if(this->source == SRC_BLOB || this->source == SRC_FILE) {
        free(this->buff);
    }

    this->buff_size = other.buff_size;
    this->max_chunk_size = other.max_chunk_size;
    this->source = other.source;

    if(this->source == SRC_CHUNK) {
        this->buff = other.buff;
    } else {
        if(other.buff) {
            this->buff = (VOCAB_DTYPE *)malloc(sizeof(VOCAB_DTYPE) * other.buff_size);
            if(this->buff == NULL) {
                throw std::runtime_error("Failed to allocate memory for data");
            }
            std::copy(other.buff, other.buff + other.buff_size, this->buff);
            this->source = SRC_BLOB;
        } else {
            this->buff = nullptr;
        }
    }

    return *this;
}

void Data::swap(Data &other) {
    VOCAB_DTYPE *tmp_buff = this->buff;
    this->buff = other.buff;
    other.buff = tmp_buff;

    size_t tmp_buff_size = this->buff_size;
    this->buff_size = other.buff_size;
    other.buff_size = tmp_buff_size;

    size_t tmp_max_chunk_size = this->max_chunk_size;
    this->max_chunk_size = other.max_chunk_size;
    other.max_chunk_size = tmp_max_chunk_size;

    enum DataSource tmp_source = this->source;
    this->source = other.source;
    other.source = tmp_source;
}
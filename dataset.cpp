#include <vector>
#include <pthread.h>

#include <data.hpp>
#include <dataset.hpp>
#include <stdexcept>

Dataset::Dataset(size_t chunk_size = CHUNK_SIZE) {
    this->cur = 0;
    pthread_mutex_init(&this->lock, NULL);
}

Dataset::~Dataset() {
    pthread_mutex_destroy(&this->lock);
}


void Dataset::prepare_chunks() {
    //Before executing this make sure the tokeniser has been run on the data
    pthread_mutex_lock(&this->lock);

    chunk_views.clear();
    for (size_t i = 0; i < data.size(); i++) {
        for (size_t j = 0; j < data[i].chunks(); j++) {
            Data chunk = data[i].get_chunk(j);
            this->chunk_views.push_back(chunk);
        }
    }
    this->cur = 0;

    pthread_mutex_unlock(&this->lock);
}

void Dataset::shuffle() {
    for (size_t i = this->chunk_views.size() - 1; i > 0; i--) {
        size_t j = rand() % i;
        //Swap the data
        this->chunk_views[i].swap(this->chunk_views[j]);
    }
}

Data *Dataset::yeild() {
    size_t idx;
    pthread_mutex_lock(&this->lock);
    idx = this->cur;
    this->cur+=1;
    pthread_mutex_unlock(&this->lock);

    if(this->cur > this->chunk_views.size()) {
        fprintf(stdout, "Out of data\n");
        fflush(stdout);
        throw std::runtime_error("Out of data");
    }

    return this->operator[](idx);
}

size_t Dataset::size() {
    return this->chunk_views.size();
}

const Data* Dataset::operator[] (size_t idx) const {
    return &this->chunk_views[idx];
}

Data* Dataset::operator[] (size_t idx) {
    return &this->chunk_views[idx];
}

DatasetFiles::DatasetFiles(size_t chunk_size, std::vector<char *> filepaths) {
    for (char *filepath: filepaths) {
        this->data.push_back(Data(filepath, chunk_size));
    }
}

DatasetFiles::~DatasetFiles() {
}
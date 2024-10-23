#include <cstddef>
#include <cstring>
#include <utility>
#include <stdexcept>
#include <cassert>

#include <omp.h>

#include <frequency.hpp>
#include <config.hpp>


Frequency::Frequency(size_t max_size=1024)
{
    frequency = new size_t*[max_size];
    for(size_t i = 0; i < max_size; i++) {
        frequency[i] = nullptr;
    }
    this->max_size = max_size;
    this->len = 0;
}

Frequency::~Frequency()
{
    for(size_t i = 0; i < max_size; i++) {
        if(frequency[i] != nullptr) {
            delete[] frequency[i];
        }
    }
    delete[] frequency;
}

const size_t& Frequency::operator() (VOCAB_DTYPE b1, VOCAB_DTYPE b2) const {
    assert(b1 < this->max_size &&  b2 < this->max_size);
    if(this->frequency[b1] == nullptr) {
        this->frequency[b1] = new size_t[this->max_size];
        memset(this->frequency[b1], 0, this->max_size * sizeof(size_t));
    }
    return this->frequency[b1][b2];
}

size_t& Frequency::operator() (VOCAB_DTYPE b1, VOCAB_DTYPE b2) {
    if(b1 >= this->max_size || b2 >= this->max_size) {
        printf("Index out of bounds: %lu, %lu\n", b1, b2);
    }
    assert(b1 < this->max_size &&  b2 < this->max_size);
    if(this->frequency[b1] == nullptr) {
        this->frequency[b1] = new size_t[this->max_size];
        memset(this->frequency[b1], 0, this->max_size * sizeof(size_t));
    }
    return this->frequency[b1][b2];
}

void Frequency::get_max_pair(VOCAB_DTYPE *max_pair) {
    
    max_pair[0] = 0;
    max_pair[1] = 0;
    
    size_t max = 0;
    for(size_t i = 0; i < this->max_size; i++) {
        if(this->frequency[i] == nullptr) {
            continue;
        }
        for(size_t j = 0; j < this->max_size; j++) {
            if(this->frequency[i][j] > max) {
                max_pair[0] = i;
                max_pair[1] = j;

                max = this->frequency[i][j];
            }
        }
    }
    return;
}

void Frequency::zero() {
    for(size_t i = 0; i < this->max_size; i++) {
        if(this->frequency[i] == nullptr) {
            continue;
        }
        memset(this->frequency[i], 0, this->max_size * sizeof(size_t));
    }
    return;
}

void Frequency::add(Frequency &other) {
    for(size_t i = 0; i < this->max_size; i++) {
        if(this->frequency[i] == nullptr && other.frequency[i] == nullptr) {
            continue;
        }
        if(this->frequency[i] == nullptr) {
            this->frequency[i] = new size_t[this->max_size];
            memcpy(this->frequency[i], other.frequency[i], this->max_size * sizeof(size_t));
            continue;
        }

        #pragma omp simd
        for(size_t j = 0; j < this->max_size; j++) {
            this->frequency[i][j] += other(i, j);
        }
    }
    return;
}
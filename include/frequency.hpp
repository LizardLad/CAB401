#ifndef FREQUENCY_HPP
#define FREQUENCY_HPP

#include <cstddef>
#include <config.hpp>
#include <utility>

class Frequency
{
    private:
        size_t **frequency;
        size_t max_size;
        size_t len;
    public:
        Frequency(size_t max_size);
        ~Frequency();
        
        size_t& operator() (VOCAB_DTYPE b1, VOCAB_DTYPE b2);
        const size_t& operator() (VOCAB_DTYPE b1, VOCAB_DTYPE b2) const;

        void add(Frequency &other);

        void get_max_pair(VOCAB_DTYPE *max_pair);
        void zero();
};

#endif
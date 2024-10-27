#ifndef DATASET_HPP
#define DATASET_HPP

#include <vector>

#include <tokeniser.hpp>
#include <config.hpp>
#include <data.hpp>

class Dataset
{
    protected:
        std::vector<Data> chunk_views; //Views of the data and not the actual data

        size_t cur;
        pthread_mutex_t lock;

        size_t chunk_size;

    public:
        std::vector<Data> data;

        Dataset(size_t chunk_size = CHUNK_SIZE);
        ~Dataset();

        void shuffle();
        void prepare_chunks();

        const Data* operator[] (size_t idx) const;
        Data* operator[] (size_t idx);

        Data *yeild();
        size_t size();
};

class DatasetFiles : public Dataset 
{
    public:
        DatasetFiles(size_t chunk_size, std::vector<char *> filepaths);
        ~DatasetFiles();
};

#endif
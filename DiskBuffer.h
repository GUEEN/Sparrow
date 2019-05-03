#pragma once

#include <vector>
#include <fstream>
#include <string>

#include "LabeledData.h"

struct BitMap {
    int size;
    std::vector<int> is_free;

    BitMap(int size, bool all_full);

    int get_first_free();
    void mark_free(int position);
    void mark_filled(int position);
    // binary integral logarithm
    static int log(int64_t t);
};

class DiskBuffer {
public:
    DiskBuffer(const std::string& filename, int feature_size, int num_examples_per_block, int capacity);
    DiskBuffer(const DiskBuffer&) = default;
    DiskBuffer(DiskBuffer&&) = default;

    ~DiskBuffer();

    int write(const std::vector<char>& data);
    std::vector<char> read(int position);

    int write_block(const std::vector<ExampleInSampleSet>& data);
    std::vector<ExampleInSampleSet> read_block(int position);

private:
    BitMap bitmap;
    int feature_size;
    int num_examples_per_block;
    int block_size;

    int capacity;
    int size;
    std::fstream file;
    std::string filename;

    static int get_block_size(int feature_size, int num_examples_per_block);

    template<class T>
    T read_element() {
        T value;
        file.read((char *)&value, sizeof(T));
        return value;
    }

    template<class T>
    void write_element(T value) {
        file.write((char *)&value, sizeof(T));
    }

};


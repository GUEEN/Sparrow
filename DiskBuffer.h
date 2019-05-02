#pragma once

#include <vector>
#include <fstream>
#include <string>

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
    DiskBuffer(const std::string& filename, int block_size, int capacity);
    DiskBuffer(const DiskBuffer&) = default;
    DiskBuffer(DiskBuffer&&) = default;

    ~DiskBuffer();

    int write(const std::vector<char>& data);
    std::vector<char> read(int position);

private:
    BitMap bitmap;
    int block_size;
    int capacity;
    int size;
    std::fstream file;
    std::string filename;
};


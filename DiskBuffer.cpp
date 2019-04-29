#include "DiskBuffer.h"

#include <cassert>

BitMap::BitMap(int size, bool all_full) {
    int vec_size = (size + 31) / 32;
    int value = all_full ? 0 : -1;

    is_free = std::vector<int>(value, vec_size);
}

// -1 corresponds to None
int BitMap::get_first_free() {
    int i = 0;
    int j = 0;
    while ((i * 32) < size) {
        int64_t k = is_free[i];

        if (k == 0) {
            ++i;
        } else {
            j = k & -k;
            break;
        }
    }
    int ret = i * 32 + BitMap::log(j);
    if (j == 0 || ret >= size) {
        return -1;
    } else {
        return ret;
    }
}

void BitMap::mark_free(int position) {
    assert(position < size);

    int div = position / 32;
    int res = position % 32;
    is_free[div] |=  (1 << res);
}

void BitMap::mark_filled(int position) {
    assert(position < size);

    int div = position / 32;
    int res = position % 32;
    is_free[div] &= !(1 << res);
}

int BitMap::log(int64_t t) {
    int left = 0;
    int right = 32 + 1;

    while (left + 1 < right) {
        int mid = (left + right) >> 1;
        if (t >> mid == 0) {
            right = mid;
        } else {
            left = mid;
        }
    }
    return left;
}

DiskBuffer::DiskBuffer(
    const std::string& filename, int block_size, int capacity) : block_size(block_size),
    capacity(capacity), filename(filename), size(0), bitmap(capacity, false) {
    file.open(filename, std::ios::binary);
    //let file = OpenOptions::new()
    //.read(true)
    //.write(true)
    //.create(true)
    //.open(filename).expect(
    //    &format!("Cannot create the buffer file at {}", filename));
}

DiskBuffer::~DiskBuffer() {
    file.close();
    remove(filename.c_str());
}



int DiskBuffer::write(const std::vector<char>& data) {
    assert(data.size() == block_size);

    int idx = bitmap.get_first_free();
    bitmap.mark_filled(idx);

    //let position = {
    //    let idx = self.bitmap.get_first_free().expect(
    //        "No free slot available."
    //    );
    //self.bitmap.mark_filled(idx);
    //idx
    //};
    int position = idx;

    assert(position <= size);

    if (position >= size) {
        ++size;
    }
    assert(size <= capacity);

    int offset = position * block_size;

    file.seekg(offset);
    file.write(data.data, data.size());
    file.flush();
    
    return position;
}

std::vector<char> DiskBuffer::read(int position) {
    assert(position < size);

    int offset = position * block_size;

    file.seekg(offset);

    std::vector<char> block_buffer(0, block_size);

    file.read(block_buffer.data, block_size);

    position = file.tellg();

    bitmap.mark_free(position);
    return block_buffer;
}


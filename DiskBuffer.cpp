#include "DiskBuffer.h"

#include <cassert>
#include <iostream>

BitMap::BitMap(int size, bool all_full) : size(size) {
    int vec_size = (size + 31) / 32;
    int value = all_full ? 0 : -1;

    is_free = std::vector<int>(vec_size, value);
}

// -1 corresponds to None
int BitMap::get_first_free() {
    int i = 0;
    int64_t j = 0;
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

    is_free[div] &= ~(1 << res);
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
    const std::string& filename, int feature_size, int num_examples_per_block, int capacity) : bitmap(capacity, false), 
    feature_size(feature_size), num_examples_per_block(num_examples_per_block),
    capacity(capacity), size(0), filename(filename) {
    block_size = get_block_size(feature_size, num_examples_per_block);

    file.open(filename, std::ios::binary | std::ios::in | std::ios::out | std::ios::app);
    file.seekp(0);
    file.seekg(0);

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
    //assert(data.size() == block_size);

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

    file.seekp(offset);
    file.write(data.data(), data.size());
    file.flush();
    
    return position;
}

std::vector<char> DiskBuffer::read(int position) {
    assert(position < size);

    int64_t offset = position * block_size;

    file.seekg(offset);

    std::vector<char> block_buffer(0, block_size);

    file.read(block_buffer.data(), block_size);

    position = file.tellg();

    bitmap.mark_free(position);
    return block_buffer;
}

int DiskBuffer::write_block(const std::vector<ExampleInSampleSet>& data) {
    //assert(data.size() == block_size);
    std::lock_guard<std::mutex> lock(mutex);

    int idx = bitmap.get_first_free();

    if (idx == -1) {
        std::cout << "No free slot available" << std::endl;
        return -1;
    }

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

    file.seekp(offset);
    for (int i = 0; i < num_examples_per_block; ++i) {
        for (int j = 0; j < feature_size; ++j) {
            write_element<TFeature>(data[i].first.feature[j]);
        }
        write_element<TLabel>(data[i].first.label);

        write_element<double>(data[i].second.first);
        write_element<int>(data[i].second.second);
    }

    file.sync();
    file.flush();

    return position;
}

std::vector<ExampleInSampleSet> DiskBuffer::read_block(int position) {
    std::lock_guard<std::mutex> lock(mutex);
    assert(position < size);

    int64_t offset = position * block_size;

    file.seekg(offset);

    std::vector<ExampleInSampleSet> buffer;

    for (int i = 0; i < num_examples_per_block; ++i) {
        std::vector<TFeature> features(feature_size);
        for (int j = 0; j < feature_size; ++j) {
            features[j] = read_element<TFeature>();
        }
        int label = read_element<TLabel>();
        
        double score = read_element<double>();
        int version = read_element<int>();

        Example ex(features, label);

        buffer.emplace_back(ex, std::make_pair( score, version));
    }
    bitmap.mark_free(position);
    return buffer;
}


// size in bytes of ExampleWithScore
// size of Example + size of int + size of double
// size of Example = size of features + size of labels

const int single_feature_size = sizeof(TFeature);
const int label_size = sizeof(TLabel);
const int score_size = sizeof(double) + sizeof(int);

int DiskBuffer::get_block_size(int feature_size, int num_examples_per_block) {
    //Example example(std::vector<TFeature>(feature_size), -1);
    //ExampleWithScore example_with_score = std::make_pair(example, std::make_pair(0.0, 0));
    //std::vector<ExampleWithScore> block(num_examples_per_block, example_with_score);

    //return sizeof(block);
    //let block : Block = vec![example_with_score; num_examples_per_block];
    //let serialized_block : Vec<u8> = serialize(&block).unwrap();
    //serialized_block.len()

    return (single_feature_size * feature_size + label_size + score_size) * num_examples_per_block;

}

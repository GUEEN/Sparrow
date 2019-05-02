#include "Strata.h"

#include <cassert>
#include <mutex>

using std::lock_guard;

int get_block_size(int feature_size, int num_examples_per_block) {
    Example example(std::vector<TFeature>(feature_size), -1);
    ExampleWithScore example_woth_score = std::make_pair(example, std::make_pair(0.0, 0));
    std::vector<ExampleWithScore> block(num_examples_per_block, example_woth_score);

    return sizeof(block);
    //let block : Block = vec![example_with_score; num_examples_per_block];
    //let serialized_block : Vec<u8> = serialize(&block).unwrap();
    //serialized_block.len()
}

DiskBuffer* get_disk_buffer(
    const std::string& filename,
    int feature_size,
    int num_examples,
    int num_examples_per_block) {
    int num_disk_block = (num_examples + num_examples_per_block - 1) / num_examples_per_block;
    int block_size = get_block_size(feature_size, num_examples_per_block);
    return new DiskBuffer(filename, block_size, num_disk_block);
}


void Strata::send(int index, const Example& example, double score, int version) {
    assert(index >= -128 && index <= 127);
    index += 128;

    lock_guard<std::mutex> lock(this->mutex);
    auto& sender = in_queues[index];
    if (sender.get() == nullptr) {
        this->create(index);
    }
    sender->send({ example, { score, version } });
}

void Strata::create(int index) {
    // TODO!!!
    assert(false);
}


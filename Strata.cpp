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


void stratum_block_read_thread(
    Receiver<ExampleWithScore>& in_queue_r_,
    Sender<ExampleWithScore>& out_queue_s_,
    Receiver<int>& slot_r,
    DiskBuffer& disk_buffer) {
    std::vector<ExampleInSampleSet> out_block;
    int index = 0;

    while (true) {
        if (index >= out_block.size()) {
            std::pair<bool, int> block_index_try = slot_r.try_recv();
            if (block_index_try.first) {
                int& block_index = block_index_try.second;

                std::vector<char> block_data = disk_buffer.read(block_index);
                // deseialize block_data
                // write deserialized block_data to  out_block
                index = 0;
                // if is some
                out_queue_s_.send(out_block[index++]);
            } else {
                // if the number of examples is less than what requires to form a block,
                // they would stay in `in_queue` forever and never write to disk.
                // We read from `in_queue` directly in this case.
                ExampleWithScore example = in_queue_r_.recv();
                // if is some
                out_queue_s_.send(example);
            }
        }
        // send if some
        out_queue_s_.send(out_block[index++]);
    }
}

void stratum_block_write_thread(int num_examples_per_block,
    Receiver<ExampleWithScore>& in_queue_r_,
    Sender<int>& slot_s,
    DiskBuffer& disk_buffer) {
    while (true) {
        if (in_queue_r_.len() >= num_examples_per_block) {

            std::vector<ExampleWithScore> in_block;
            for (int i = 0; i < num_examples_per_block; ++i) {
                in_block.push_back(in_queue_r_.recv());
            }

            std::vector<char> serialized_block; // = serialize(in_block);

            int slot_index = disk_buffer.write(serialized_block);
            slot_s.send(slot_index);
        }
        //else {
        //    //sleep(Duration::from_millis(100));
        //}
    }
}


Stratum::Stratum(
    int index,
    int num_examples_per_block,
    DiskBuffer& disk_buffer) : in_channel(bounded_channel<ExampleWithScore>(num_examples_per_block * 2, "stratum-i" + std::to_string(index))),
    slot_channel(unbounded_channel<int>("stratum-slot" + std::to_string(index))),
    out_channel(bounded_channel<ExampleWithScore>(num_examples_per_block * 2, "stratum-o" + std::to_string(index))) {

    Sender<ExampleWithScore>& in_queue_s = in_channel.first;
    Receiver<ExampleWithScore>& in_queue_r = in_channel.second;

    Sender<int>& slot_s = slot_channel.first;
    Receiver<int>& slot_r = slot_channel.second;

    Sender<ExampleWithScore>& out_queue_s = out_channel.first;
    Receiver<ExampleWithScore>& out_queue_r = out_channel.second;

    std::thread thbw(stratum_block_write_thread, std::ref(num_examples_per_block), 
        std::ref(in_queue_r), std::ref(slot_s), std::ref(disk_buffer));
    thbw.detach();

    std::thread thbr(stratum_block_read_thread, std::ref(in_queue_r), 
        std::ref(out_queue_s), std::ref(slot_r), std::ref(disk_buffer));
    thbr.detach();
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


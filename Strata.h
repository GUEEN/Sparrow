#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Channels.h"
#include "DiskBuffer.h"


DiskBuffer* get_disk_buffer(
    const std::string& filename,
    int feature_size,
    int num_examples,
    int num_examples_per_block);

struct Stratum {
    Stratum(int index,
        int num_examples_per_block,
        std::unique_ptr<DiskBuffer>& disk_buffer);

    std::pair<Sender<ExampleWithScore>, Receiver<ExampleWithScore>> in_channel;
    std::pair<Sender<ExampleWithScore>, Receiver<ExampleWithScore>> out_channel;
    std::pair<Sender<int>, Receiver<int>> slot_channel;
    
    /*InQueueSender in_queue_s;
    OutQueueReceiver out_queue_r;*/
};    

struct Strata {
    int num_examples_per_block;
    std::unique_ptr<DiskBuffer> disk_buffer; // !!! Arc<RwLock<DiskBuffer>>,

    HashMapSenders in_queues; // !!! Arc<RwLock<HashMapSenders>>,
    HashMapReceiver out_queues; // !!! Arc<RwLock<HashMapReceiver>>

    std::mutex mutex;

    Strata(
        int num_examples,
        int feature_size,
        int num_examples_per_block,
        const std::string& disk_buffer_name)
        : num_examples_per_block(num_examples_per_block),
        disk_buffer(
            get_disk_buffer(
                disk_buffer_name, feature_size, num_examples, num_examples_per_block)) {
    }

    void send(int index, const Example& example, double score, int version);
    
    std::unique_ptr<InQueueSender>& get_in_queue(int index);
    std::unique_ptr<OutQueueReceiver>& get_out_queue(int index);

private:
    std::pair<InQueueSender, OutQueueReceiver> create(int index);
};

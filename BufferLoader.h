#pragma once

#include <vector>

#include "Channels.h"
#include "Tree.h"

void fill_buffer(
    int new_sample_capacity,
    std::vector<ExampleWithScore>& new_sample_buffer,
    Receiver<std::pair<ExampleWithScore, int>>& gather_new_sample
);

struct Gatherer {

    Gatherer(Receiver<std::pair<ExampleWithScore, int>>& gather_new_sample,
        std::vector<ExampleWithScore>& new_sample_buffer,
        int new_sample_capacity);

    void run(bool blocking);

    Receiver<std::pair<ExampleWithScore, int>> gather_new_sample;
    std::vector<ExampleWithScore> new_sample_buffer;
    int new_sample_capacity;
};


class BufferLoader {
public:
    BufferLoader(
        int size,
        int batch_size,
        Receiver<std::pair<ExampleWithScore, int>>&  gather_new_sample,
        Sender<Signal>& sampling_signal_channel,
        bool serial_sampling,
        bool init_block,
        double min_ess
    );

    std::vector<ExampleInSampleSet> get_next_batch(bool allow_switch);

    std::vector<ExampleInSampleSet> get_next_batch_and_update(
        bool allow_switch,
        Model& model
    );

    std::vector<ExampleInSampleSet> get_next_mut_batch(bool allow_switch);

    int get_num_batches() const;
    bool try_switch();
    void update_ess();

private:
    int size;
    int batch_size;
    int num_batch;

    std::vector<ExampleInSampleSet> examples;
    std::vector<ExampleInSampleSet> new_examples;

    Gatherer gatherer;
        
    bool serial_sampling;
    
    double ess;
    double min_ess;
    int curr_example;
};

void update_scores(std::vector<ExampleInSampleSet>& data, Model& model);

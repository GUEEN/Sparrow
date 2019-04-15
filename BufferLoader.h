#pragma once

#include "Utils.h"

class BufferLoader {
public:
    BufferLoader(
        int size,
        int batch_size,
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
        
    bool serial_sampling;
    
    double ess;
    double min_ess;
    int curr_example;
}

void update_scores(std::vector<ExampleInSampleSet>& data, Model& model);

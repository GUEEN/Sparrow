#include "BufferLoader.h"

#include <iostream>
#include <cassert>
#include <algorithm>
#include <vector>

#include "Utils.h"
#include "ThreadManager.h"


void fill_buffer(
    int new_sample_capacity,
    std::vector<ExampleWithScore>& new_sample_buffer,
    Receiver<std::pair<ExampleWithScore, int>>& gather_new_sample
) {

    //std::cout << "Start filling the alternate buffer" << std::endl;

    //new_sample_buffer.reserve(new_sample_capacity);

    while (new_sample_buffer.size() < new_sample_capacity) {
        std::pair<ExampleWithScore, int> excc = gather_new_sample.recv();
        ExampleWithScore example = excc.first;
        int c = excc.second;
        // `c` is the number of times this example should be put into the sample set
        while (new_sample_buffer.size() < new_sample_capacity && c > 0) {
            new_sample_buffer.emplace_back(example);
            c--;
        }
    }
}

void gatherer_thread(
    int new_sample_capacity,
    std::vector<ExampleWithScore>& new_sample_buffer,
    Receiver<std::pair<ExampleWithScore, int>>& gather_new_sample) {

    while (ThreadManager::continue_run) {
        fill_buffer(new_sample_capacity, new_sample_buffer, gather_new_sample);
    }
}

Gatherer::Gatherer(
    Receiver<std::pair<ExampleWithScore, int>>& gather_new_sample,
    std::vector<ExampleWithScore>& new_sample_buffer,
    int new_sample_capacity
) : gather_new_sample(gather_new_sample), new_sample_buffer(new_sample_buffer),
new_sample_capacity(new_sample_capacity) { }

/// Start the gatherer.
///
/// Fill the alternate memory buffer of the buffer loader
void Gatherer::run(bool blocking) {

    if (blocking) {
        std::cout << "Starting blocking gatherer" << std::endl;
        fill_buffer(new_sample_capacity, new_sample_buffer, gather_new_sample);
    }
    else {
        std::cout << "Starting non-blocking gatherer" << std::endl;
        std::thread th(gatherer_thread, new_sample_capacity, std::ref(new_sample_buffer), std::ref(gather_new_sample));
        th.detach();
    }
}


BufferLoader::BufferLoader(
    int size,
    int batch_size,
    Receiver<std::pair<ExampleWithScore, int>>&  gather_new_sample,
    Sender<Signal>& sampling_signal_channel,
    bool serial_sampling,
    bool init_block,
    double min_ess) : size(size), batch_size(batch_size),
    gather_new_sample(gather_new_sample),
    sampling_signal_channel(sampling_signal_channel),
    serial_sampling(serial_sampling),
    gatherer(gather_new_sample, new_examples, size),
    ess(0.0), min_ess(min_ess), curr_example(0) {
    num_batch = (size + batch_size - 1) / batch_size;
    if (serial_sampling == false) {
        sampling_signal_channel.send(Signal::START);
    }
    if (init_block) {
        force_switch();
    }
    if (serial_sampling == false) {
        gatherer.run(false);
    }
}


int BufferLoader::get_num_batches() const {
    return num_batch;
}

std::vector<ExampleInSampleSet> BufferLoader::get_next_batch(bool allow_switch) {
    return get_next_mut_batch(allow_switch);
}

std::vector<ExampleInSampleSet> BufferLoader::get_next_batch_and_update(bool allow_switch, std::shared_ptr<Model>& model) {
    std::vector<ExampleInSampleSet> batch = get_next_mut_batch(allow_switch);
    update_scores(batch, model);
    return batch;
}

std::vector<ExampleInSampleSet> BufferLoader::get_next_mut_batch(bool allow_switch) {
    if (ess <= min_ess && allow_switch && !serial_sampling) {
        try_switch();
    }

    curr_example += batch_size;
    if (curr_example >= size) {
        update_ess();
        curr_example = 0;
    }

    assert(!examples.empty());
    int tail = std::min(curr_example + batch_size, size);

    return std::vector<ExampleInSampleSet>(examples.begin() + curr_example, examples.begin() + tail);
}

void BufferLoader::force_switch() {
    std::cout << "Force-switch started." << std::endl;
    if (serial_sampling) {
        sampling_signal_channel.send(Signal::START);
    }
    gatherer.run(true);
    if (serial_sampling) {
        sampling_signal_channel.send(Signal::STOP);
    }

    assert(try_switch());
    std::cout << "Force-switch quit." << std::endl;
}

bool BufferLoader::try_switch() {

    bool switched = false;

    if (!new_examples.empty()) {
        examples.clear();
        for (const ExampleInSampleSet& ex : new_examples) {
            const Example& a = ex.first;
            std::pair<double, int> s = ex.second;

            double w = get_weight(a, 0.0);
            examples.emplace_back(a, std::make_pair(w, s.second));
        }

        curr_example = 0;
        update_ess();
        switched = true;
    }

    return switched;
}


void BufferLoader::update_ess() {
    double sum_weights = 0.0;
    double sum_weights_squared = 0.0;

    for (const ExampleInSampleSet& ex : examples) {
        double w = ex.second.first;

        sum_weights += w;
        sum_weights_squared += w*w;
    }

    ess = sum_weights * sum_weights / sum_weights_squared / size;
    if (serial_sampling && ess < min_ess) {
        force_switch();
    }
}

/// Update the scores of the examples using `model`
void update_scores(std::vector<ExampleInSampleSet>& data, std::shared_ptr<Model>& model) {
    int model_size = model->size();

    for (ExampleInSampleSet& example : data) {
        double curr_weight = (example.second).first;
        double new_score = 0.0;

        for (int i = example.second.second; i < model_size; ++i) {
            new_score += (*model)[i].get_leaf_prediction(example.first);
        }

        example.second = std::make_pair(curr_weight * get_weight(example.first, new_score), model_size);
    }
}

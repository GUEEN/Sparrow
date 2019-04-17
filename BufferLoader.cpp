#include <cassert>
#include <algorithm>

#include "BufferLoader.h"


BufferLoader::BufferLoader(
    int size,
    int batch_size,
    bool serial_sampling,
    bool init_block,
    double min_ess): size(size), batch_size(batch_size), serial_sampling(serial_sampling),
    ess(0.0), min_ess(0.0), curr_example(0)  {
    num_batch = (size + batch_size - 1) / batch_size;
}


int BufferLoader::get_num_batches() const {
    return num_batch;
}

std::vector<ExampleInSampleSet> BufferLoader::get_next_batch(bool allow_switch) {
    return get_next_mut_batch(allow_switch);
}

std::vector<ExampleInSampleSet> BufferLoader::get_next_batch_and_update(bool allow_switch, Model& model) {
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
    //if (ess < min_ess) {
    //    force_switch();
    //}
}

/// Update the scores of the examples using `model`
void update_scores(std::vector<ExampleInSampleSet>& data, Model& model) {
    int model_size = model.size();

    for (ExampleInSampleSet& example : data) {
        double curr_weight = (example.second).first;
        double new_score = 0.0;

        for (int i = example.second.second; i < model_size; ++i) {
            new_score += model[i].get_leaf_prediction(example.first);
        }

        example.second = std::make_pair(curr_weight * get_weight(example.first, new_score), model_size);
    }
}

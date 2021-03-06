#pragma once

#include <vector>

#include "Learner.h"
#include "BufferLoader.h"

class Boosting {
public:
    Boosting(
        int num_iterations,
        int max_leaves,
        double min_gamma,
        int max_trials_before_shrink,
        const std::vector<Bins>& bins,
        BufferLoader& training_loader,
        int max_sample_size,
        double default_gamma,
        std::shared_ptr<Model>& model);

    void training(
        std::vector<Example> validate_set1,
        std::vector<Example> validate_set2);

private:

    int num_iterations;
    int channel_size;

    BufferLoader training_loader;
    Learner learner;

    double sum_gamma;
    double remote_sum_gamma;

    std::shared_ptr<Model> model;

};

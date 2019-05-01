#pragma once

#include "Learner.h"
#include "BufferLoader.h"

#include <vector>

class Boosting {
public:
    Boosting(
        int num_iterations,
        int max_leaves,
        double min_gamma,
        int max_trials_before_shrink,
        const std::vector<Bins>& bins,
        BufferLoader& training_loader,
        Range range,
        int max_sample_size,
        double default_gamma);

    void training(
        std::vector<Example> validate_set1,
        std::vector<Example> validate_set2);

private:

    int num_iterations;

    BufferLoader training_loader;
    Model model;
    Learner learner;

    double sum_gamma;
    double remote_sum_gamma;
};

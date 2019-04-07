#pragma once

#include "Learner.h"

class Boosting {
public:
    Boosting(
        int num_iterations,
        int max_leaves,
        double min_gamma,
        int max_trials_before_shrink,
        //bins,
        int max_sample_size,
        double default_gamma);

    void train();

private:

    int num_iterations;
    Learner learner;

    double sum_gamma;
};

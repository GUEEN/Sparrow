#include "Boosting.h"

Boosting::Boosting(int num_iterations,
    int max_leaves,
    double min_gamma,
    int max_trials_before_shrink,
    //bins,
    int max_sample_size,
    double default_gamma) : learner(max_leaves, min_gamma, default_gamma, max_trials_before_shrink)  {





}


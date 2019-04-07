#pragma once

#include "Tree.h"

class Learner {
public:
    Learner(int max_leaves,
        double min_gamma,
        double default_gamma,
        // bins
        int num_examples_before_shrink);

    void update();

private:
    // bins 

    int range_start;
    int num_examples_before_shrink;

    double rho_gamma;
    double root_rho_gamma;
    double tree_max_rho_gamma;

    std::vector<int> counts;
    std::vector<double> sum_weights;
    std::vector<bool> is_active;

    double default_gamma;
    double min_gamma;
    int num_candid;
    int pub_total_count;

    double total_weight;

    int max_leaves;

    Tree tree;

};

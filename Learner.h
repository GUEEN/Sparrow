#pragma once

#include <memory>

#include "BufferLoader.h"
#include "Bins.h"

const double GAMMA_GAP = 0.0;
const int NUM_RULES = 2;

typedef std::vector<std::vector<std::vector<std::vector<double>>>> ScoreBoard;

class Learner {
public:
    Learner(
        int max_leaves,
        double min_gamma,
        double default_gamma,
        int num_examples_before_shrink,
        const std::vector<Bins>& bins,
        Range range);

    bool is_gamma_significant() const;

    std::shared_ptr<Tree> update(
        const std::vector<ExampleInSampleSet>& data,
        const std::vector<Example>& validate_set1,
        const std::vector<double>& validate_w1,
        const std::vector<Example>& validate_set2,
        const std::vector<double>& validate_w2);

    void reset_all();
    void setup(int index);
    void reset_trackers();

    std::pair<double, std::tuple<int, int, int, int>> get_max_empirical_ratio();

private:
    std::vector<Bins> bins;

    int range_start;
    int num_examples_before_shrink;

    ScoreBoard weak_rules_score;
    ScoreBoard sum_c;
    ScoreBoard sum_c_squared;
    
    double rho_gamma;
    double root_rho_gamma;
    double tree_max_rho_gamma;

    std::vector<int> counts;
    std::vector<double> sum_weights;
    std::vector<bool> is_active;

    double default_gamma;
    double min_gamma;
    int num_candid;
    int total_count;
    double total_weight;
    int max_leaves;

    Tree tree;
};

TreeScore get_base_tree(int max_sample_size, BufferLoader& data_loader);
#include <algorithm>

#include <array>

#include "BufferLoader.h"
#include "Learner.h"

//const RULES : [[f32; 2]; NUM_RULES] = [[1.0, -1.0], [-1.0, 1.0]];

Learner::Learner(
        int max_leaves,
        double min_gamma,
        double default_gamma,
        int num_examples_before_shrink,
        const std::vector<Bins>& bins,
        Range range
        ) : max_leaves(max_leaves), min_gamma(min_gamma), default_gamma(default_gamma),
    rho_gamma(default_gamma), root_rho_gamma(default_gamma), range_start(range.start), 
    tree_max_rho_gamma(0.0), num_examples_before_shrink(num_examples_before_shrink),
    bins(bins), weak_rules_score(bins.size()), sum_c(bins.size()), sum_c_squared(bins.size()),
    tree(2 * max_leaves - 1) {

    setup(0);
}

void Learner::reset_all() {
    reset_trackers();
    //self.is_active.iter_mut().for_each(| t | { *t = false; });
    num_candid = 0;
    tree_max_rho_gamma = 0.0;
    setup(0);
    rho_gamma = root_rho_gamma;
}

/// Reset the statistics of the speicified candidate weak rules
/// Trigger when the gamma is changed and new node is added
void Learner::reset_trackers() {
    for (int i = 0; i < bins.size(); ++i) {
        for (int index = 0; index < num_candid; ++index) {
            if (is_active[index]) {
                for (int j = 0; j < bins[i].get_size(); ++j) {
                    for (int k = 0; k < NUM_RULES; ++k) {
                        weak_rules_score[i][index][j][k] = 0.0;
                        sum_c[i][index][j][k] = 0.0;
                        sum_c_squared[i][index][j][k] = 0.0;
                    }
                }
            }
        }
    }
    for (int index = 0; index < num_candid; ++index) {
        sum_weights[index] = 0.0;
        counts[index] = 0;
    }
    total_count = 0;
    total_weight = 0.0;
}

void Learner::setup(int index) {
    bool is_cleared;
    while (index >= is_active.size()) {
        if (is_active.size() == index) {
            is_cleared = true;
        }

        std::vector<double> zeros(NUM_RULES);

        for (int i = 0; i < bins.size(); ++i) {
            int len = bins[i].get_size();
            weak_rules_score[i].emplace_back(len, zeros);
            sum_c[i].emplace_back(len, zeros);
            sum_c_squared[i].emplace_back(len, zeros);
        }

        sum_weights.push_back(0.0);
        counts.push_back(0);
        is_active.push_back(false);
    }
    if (is_cleared) {
        for (int i = 0; i < bins.size(); ++i) {
            for (int j = 0; j < bins[i].get_size(); ++j) {
                for (int k = 0; k < NUM_RULES; ++k) {
                    weak_rules_score[i][index][j][k] = 0.0;
                    sum_c[i][index][j][k] = 0.0;
                    sum_c_squared[i][index][j][k] = 0.0;
                }
            }
        }
        sum_weights[index] = 0.0;
        counts[index] = 0;
    }

    is_active[index] = true;
    num_candid = std::max(num_candid, index + 1);
    rho_gamma = default_gamma;
}

std::pair<double, std::tuple<int, int, int, int>> Learner::get_max_empirical_ratio() {
    std::vector<int> indices;

    for (int i = 0; i < is_active.size(); ++i) {
        if (is_active[i])
            indices.push_back(i);
    }

    //let indices : Vec<usize> = self.is_active.iter().enumerate()
    //    .filter(| (_, is_active) | **is_active)
    //    .map(| (index, _) | index)
    //    .collect();

    double max_ratio = 0.0;
    double actual_ratio = 0.0;
    std::tuple<int, int, int, int> rule_id;

    for (int i = 0; i < bins.size(); ++i) {
        for (int index : indices) {
            for (int j = 0; j < bins[i].len(); ++j) {
                for (int k = 0; k < NUM_RULES; ++k) {
                    // max ratio considers absent examples, actual ratio does not
                    double ratio = weak_rules_score[i][index][j][k] / total_weight;
                    if (ratio >= max_ratio) {
                        max_ratio = ratio;
                        actual_ratio = weak_rules_score[i][index][j][k] / sum_weights[index];
                        rule_id = std::make_tuple(i, j, index, k);
                    }
                }
            }
        }
    }
    return std::make_pair(actual_ratio, rule_id);
}

bool Learner::is_gamma_significant() const {
    return tree_max_rho_gamma >= min_gamma || root_rho_gamma >= min_gamma;
}


TreeScore get_base_tree(int max_sample_size, BufferLoader& data_loader) {
    int sample_size = max_sample_size;
    int n_pos = 0;
    int n_neg = 0;

    while (sample_size > 0) {
        auto data = data_loader.get_next_batch(true);
        
        //let(num_pos, num_neg) =
        //    data.par_iter().fold(
        //        || (0, 0),
        //        | (num_pos, num_neg), (example, _) | {
        //    if example.label > 0 {
        //        (num_pos + 1, num_neg)
        //    }
        //    else {
        //        (num_pos, num_neg + 1)
        //    }
        //}
        //).reduce(|| (0, 0), | (a1, a2), (b1, b2) | (a1 + b1, a2 + b2));

        //n_pos += num_pos;
        //n_neg += num_neg;
        sample_size -= data.size();
    }

    double gamma = std::fabs((0.5 - (n_pos * 1.0) / (n_pos + n_neg)));
    double prediction = log(0.5 * ((n_pos * 1.0) / n_neg));

    Tree tree(2);
    tree.split(0, 0, 0, prediction, prediction);
    tree.release();

    return std::make_pair(tree, gamma);
}

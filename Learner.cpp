#include "BufferLoader.h"
#include "Learner.h"
#include "Utils.h"

#include <cmath>
#include <algorithm>
#include <unordered_map>

const double RULES[2][NUM_RULES] = { {1.0, -1.0}, {-1.0, 1.0} };

Learner::Learner(
        int max_leaves,
        double min_gamma,
        double default_gamma,
        int num_examples_before_shrink,
        const std::vector<Bins>& bins,
        Range range
        ) : bins(bins), range_start(range.start), 
    num_examples_before_shrink(num_examples_before_shrink), weak_rules_score(bins.size()),
    sum_c(bins.size()), sum_c_squared(bins.size()), default_gamma(default_gamma),
    min_gamma(min_gamma), rho_gamma(default_gamma), root_rho_gamma(default_gamma),
    tree_max_rho_gamma(0.0), max_leaves(max_leaves), tree(2 * max_leaves - 1) {
    setup(0);
}

void Learner::reset_all() {
    reset_trackers();
    is_active.assign(is_active.size(), false);
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


/// Update the statistics of all candidate weak rules using current batch of
/// training examples.
std::shared_ptr<Tree> Learner::update(
    const std::vector<ExampleInSampleSet>& data,
    const std::vector<Example>& validate_set1,
    const std::vector<double>& validate_w1,
    const std::vector<Example>& validate_set2,
    const std::vector<double>& validate_w2
) {
    // update global stats
    total_count += data.size();
    for (const ExampleInSampleSet& ex : data) {
        total_weight += ex.second.first;
    }

    typedef std::vector<std::vector<std::vector<double>>> trivector;
    typedef std::tuple<int, double, std::pair<Example, trivector>> Triple;

    std::vector<std::tuple<int, double, std::pair<Example, trivector>>> Data;
    for (int i = 0; i < data.size(); ++i) {
        const ExampleInSampleSet& exss = data[i];
        const Example& example = exss.first;
        double weight = exss.second.first;

        std::pair<int, double> p = tree.get_leaf_index_prediction(example);

        int index = p.first;
        double pred = p.second;

        weight *= get_weight(example, pred);
        double labeled_weight = weight * example.label;
        double null_weight = 2.0 * rho_gamma * weight;

        trivector vals(2, std::vector<std::vector<double>>(3, std::vector<double>(2)));
        for (int i = 0; i < 2; ++i) {
            double left_abs_val = RULES[i][0] * labeled_weight;
            double left_ci = left_abs_val - null_weight;
            double right_abs_val = RULES[i][1] * labeled_weight;
            double right_ci = right_abs_val - null_weight;

            vals[i][0][0] = left_abs_val;
            vals[i][0][1] = right_abs_val;
            vals[i][1][0] = left_ci;
            vals[i][1][1] = right_ci;
            vals[i][2][0] = left_ci * left_ci;
            vals[i][2][1] = right_ci * right_ci;
        }

        Data.push_back(std::make_tuple(index, weight, std::make_pair(example, vals)));
    }


    std::unordered_map<int, std::vector<std::pair<Example, trivector>>> data_by_node;
    // Sort examples - Complexity: O(Examples)
    for (const Triple& trip : Data) {
        int index = std::get<0>(trip);
        double weight = std::get<1>(trip);
        std::pair<Example, trivector> value = std::get<2>(trip);

        sum_weights[index] += weight;
        counts[index]++;

        data_by_node[index].push_back(value);
    }

    // Update each weak rule - Complexity: O(Candid * Bins * Splits)
    std::shared_ptr<TreeNode> valid_tree_node; // = None;

    for (int index = 0; index < num_candid; ++index) { // Splitting node candidate index
        if (data_by_node.find(index) != data_by_node.end()) {
            continue;
        }

        std::vector<std::pair<Example, trivector>> data = data_by_node[index];

        std::shared_ptr<TreeNode> tree_node;

        for (int i = 0; i < bins.size(); ++i) {

            Bins bin = bins[i];

            auto& weak_rules_score_ = weak_rules_score[i];
            auto& sum_c_ = sum_c[i];
            auto& sum_c_squared_ = sum_c_squared[i];

            // <Split, NodeId, RuleId, stats, LeftOrRight>
            // the last element of is for the examples that are larger than all split values
            std::vector<trivector> bin_accum_vals(bin.len() + 1);

            for (const std::pair<Example, trivector>& pp : data) {
                Example example = pp.first;
                trivector values = pp.second;

                int flip_index = example.feature[range_start + i];

                for (int j = 0; j < NUM_RULES; ++j) {
                    for (int k = 0; k < 3; ++k) {
                        bin_accum_vals[flip_index][j][k][0] += values[j][k][0];
                        bin_accum_vals[flip_index][j][k][1] += values[j][k][1];
                    }
                }
            }

            std::vector<std::vector<double>> accum_left(NUM_RULES, std::vector<double>(3));
            std::vector<std::vector<double>> accum_right(NUM_RULES, std::vector<double>(3));

            // Accumulate sum of the stats of all examples that go to the right child
            for (int j = 0; j < bin.len(); ++j) { // Split value
                for (int rule_idx = 0; rule_idx < NUM_RULES; ++rule_idx) { // Types of rule
                    for (int it = 0; it < 3; ++it) {  // 3 trackers
                        accum_right[rule_idx][it] += bin_accum_vals[j][rule_idx][it][1];
                    }
                }
            }
            // Now update each splitting values of the bin
            std::shared_ptr<TreeNode> valid_weak_rule; // = None;
            for (int j = 0; j < bin.len(); ++j) {
                for (int rule_idx = 0;rule_idx < NUM_RULES; ++rule_idx) { // Types of rule
                    for (int it = 0; it < 3; ++it) { // Move examples from the right to the left child
                        accum_left[rule_idx][it] += bin_accum_vals[j][rule_idx][it][0];
                        accum_right[rule_idx][it] -= bin_accum_vals[j][rule_idx][it][1];
                    }

                    double& _weak_rules_score_ = weak_rules_score_[index][j][rule_idx];
                    double& _sum_c_ = sum_c_[index][j][rule_idx];
                    double& _sum_c_squared_ = sum_c_squared_[index][j][rule_idx];

                    _weak_rules_score_ += accum_left[rule_idx][0] + accum_right[rule_idx][0];
                    _sum_c_ += accum_left[rule_idx][1] + accum_right[rule_idx][1];
                    _sum_c_squared_ += accum_left[rule_idx][2] + accum_right[rule_idx][2];

                    // Check stopping rule
                    int count = counts[index];
                    double bound = get_bound(_sum_c_, _sum_c_squared_);
                    if (_sum_c_ > bound) {
                        double base_pred = 0.5 * log((0.5 + rho_gamma + GAMMA_GAP) / (0.5 - rho_gamma - GAMMA_GAP));
                        valid_weak_rule.reset(new TreeNode({ index, rule_idx, i + range_start, j,
                            base_pred * RULES[rule_idx][0], base_pred * RULES[rule_idx][1],
                            rho_gamma, _weak_rules_score_, _sum_c_, _sum_c_squared_, bound, count, false }));
                    }
                }
            }
            // valid_weak_rule
            // }).find_any(| t | t.is_some()).unwrap_or(None)
        }

        if (valid_tree_node && tree_node) {
            valid_tree_node = tree_node;
            break;
        }
    }

    std::shared_ptr<TreeNode> tree_node;
    if (valid_tree_node || total_count <= num_examples_before_shrink) {
        tree_node = valid_tree_node;
    } else {
        // cannot find a valid weak rule, need to fallback and shrink gamma
        std::pair<double, std::tuple<int, int, int, int>> emp_ratio = get_max_empirical_ratio();

        double empirical_gamma = emp_ratio.first;
        int i = std::get<0>(emp_ratio.second);
        int j = std::get<1>(emp_ratio.second);
        int index = std::get<2>(emp_ratio.second);
        int k = std::get<3>(emp_ratio.second);

        empirical_gamma /= 2.0;
        double bounded_empirical_gamma = std::min(0.25, empirical_gamma);
        // Fallback prepare

        double base_pred = log(0.5 * ((0.5 + bounded_empirical_gamma) / (0.5 - bounded_empirical_gamma)));

        int count = counts[index];
        double raw_martingale = weak_rules_score[i][index][j][k];
        double _sum_c_ = sum_c[i][index][j][k];
        double _sum_c_squared_ = sum_c_squared[i][index][j][k];
        double bound = get_bound(_sum_c_, _sum_c_squared_);
        // shrink rho_gamma
        // let old_rho_gamma = self.rho_gamma;
        // self.rho_gamma = 0.9 * min(old_rho_gamma, empirical_gamma);
        if (is_active[0]) {
            root_rho_gamma = empirical_gamma * 0.8;
        }
        // trackers will reset later
        // debug!("shrink-gamma, {}, {}, {}",
        //         old_rho_gamma, empirical_gamma, self.rho_gamma);
        // generate a fallback tree node
        tree_node.reset(new TreeNode({ index, k, i + range_start, j, base_pred * RULES[k][0],
                        base_pred * RULES[k][1], empirical_gamma,raw_martingale, _sum_c_,
                        _sum_c_squared_, bound, count, false }));
    }

    std::shared_ptr<Tree> ret;

    if (tree_node) {

        //tree_node->write_log();

        if (validate_set1.size() > 0) {

            double mart1 = 0.0;
            double weight1 = 0.0;

            for (int i = 0; i < validate_set1.size(); ++i) {
                Example example = validate_set1[i];
                double w = validate_w1[i];

                std::pair<int, double> p = tree.get_leaf_index_prediction(example);
                int index = p.first;
                double pred = p.second;

                if (index != tree_node->tree_index) {
                    continue;

                    double weight = w * get_weight(example, pred);
                    double labeled_weight = weight * example.label;
                    double mart = 0.0;

                    if (example.feature[tree_node->feature] <= tree_node->threshold) {
                        mart = RULES[tree_node->node_type][0] * labeled_weight;
                    } else {
                        mart = RULES[tree_node->node_type][1] * labeled_weight;
                    }

                    mart1 += mart;
                    weight1 += weight;
                }
            }
            double mart2 = 0.0;
            double weight2 = 0.0;

            for (int i = 0; i < validate_set2.size(); ++i) {

                Example example = validate_set2[i];
                double w = validate_w2[i];

                std::pair<int, double> p = tree.get_leaf_index_prediction(example);
                int index = p.first;
                double pred = p.second;

                if (index != tree_node->tree_index) {
                    continue;
                }

                double weight = w * get_weight(example, pred);
                double labeled_weight = weight * example.label;
                double mart = 0.0;

                if (example.feature[tree_node->feature] <= tree_node->threshold) {
                    mart = RULES[tree_node->node_type][0] * labeled_weight;
                } else {
                    mart = RULES[tree_node->node_type][1] * labeled_weight;
                }

                mart2 += mart;
                weight2 += weight;
            }
            //std::cout << "Validate " << tree_node->fallback << ", " << tree_node->num_scanned
            //    << ",  " << tree_node->tree_index << ", " << tree_node->gamma <<
            //    ", " << mart1 / weight1 / 2.0 << ", " << mart2 / weight2 / 2.0;
        }

        std::pair<int, int> split = tree.split(
            tree_node->tree_index, tree_node->feature, tree_node->threshold,
            tree_node->left_predict, tree_node->right_predict);

        int left_node = split.first;
        int right_node = split.second;

        is_active[tree_node->tree_index] = false;
        if (tree_node->tree_index > 0) {
            // This is not the root node
            double tree_gamma = tree_node->gamma;
            if (tree_node->fallback) {
                tree_gamma *= 0.9;
            }
            tree_max_rho_gamma = std::max(tree_max_rho_gamma, tree_gamma);
        }

        reset_trackers();

        if (tree.get_num_leaves() == max_leaves * 2 - 1) {

            //debug!("default-gamma, {}, {}", self.default_gamma, self.tree_max_rho_gamma * 0.9);
            // self.default_gamma = 0.25;
            default_gamma = tree_max_rho_gamma * 0.9;
            // A new tree is created
            tree.release();
            ret.reset(new Tree(tree)); // self.tree must be cloned here

            tree = Tree(max_leaves * 2 - 1);
        } else {
            // Tracking weak rules on the new candidate leaves
            setup(left_node);
            setup(right_node);
        }
    }
    return ret;
}


TreeScore get_base_tree(int max_sample_size, BufferLoader& data_loader) {
    int sample_size = max_sample_size;
    int n_pos = 0;
    int n_neg = 0;

    while (sample_size > 0) {
        std::vector<ExampleInSampleSet> data = data_loader.get_next_batch(true);
        
        int num_pos = 0;
        int num_neg = 0;

        for (int i = 0; i < data.size(); ++i) {
            if (data[i].first.label > 0) {
                ++num_pos;
            } else {
                ++num_neg;
            }
        }
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
        n_pos += num_pos;
        n_neg += num_neg;
        sample_size -= data.size();
    }

    double gamma = std::fabs((0.5 - (n_pos * 1.0) / (n_pos + n_neg)));
    double prediction = log(0.5 * ((n_pos * 1.0) / n_neg));

    Tree tree(2);
    tree.split(0, 0, 0, prediction, prediction);
    tree.release();

    return std::make_pair(tree, gamma);
}

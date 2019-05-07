#include "Tree.h"

#include <utility>
#include <cassert>

Tree::Tree(int max_leaves) : max_leaves(max_leaves), num_leaves(0) {
    left_child.reserve(2 * max_leaves);
    right_child.reserve(2 * max_leaves);
    split_feature.reserve(2 * max_leaves);
    threshold.reserve(2 * max_leaves);
    leaf_value.reserve(2 * max_leaves);
    leaf_depth.reserve(2 * max_leaves);

    add_new_node(0.0, 0);
}

void Tree::release() {
    left_child.shrink_to_fit();
    right_child.shrink_to_fit();
    split_feature.shrink_to_fit();
    threshold.shrink_to_fit();
    leaf_value.shrink_to_fit();
    leaf_depth.shrink_to_fit();
}

std::pair<int, int> Tree::split(int leaf, int feature, int threshold_,
    double left_value, double right_value) {

    assert(split_feature[leaf] == -1);

    double leaf_value_ = leaf_value[leaf];
    int leaf_depth_ = leaf_depth[leaf];

    split_feature[leaf] = feature;
    threshold[leaf] = threshold_;
    left_child[leaf] = num_leaves;
    add_new_node(leaf_value_ + left_value, leaf_depth_ + 1);
    right_child[leaf] = num_leaves;
    add_new_node(leaf_value_ + right_value, leaf_depth_ + 1);

    return std::make_pair(left_child[leaf], right_child[leaf]);
}

std::pair<int, double> Tree::get_leaf_index_prediction(const Example& data) const {
    int node = 0;

    const std::vector<int>& feature = data.feature;

    int sfeature = split_feature[node];

    while (sfeature != -1) {
        if (feature[sfeature] <= threshold[node]) {
            node = left_child[node];
        } else {
            node = right_child[node];
        }
        sfeature = split_feature[node];
    }
    return std::make_pair(node, leaf_value[node]);
}

double Tree::get_leaf_prediction(const Example& data) const {
    return get_leaf_index_prediction(data).second;
}

void Tree::add_new_node(double leaf_value_, int depth) {
    ++num_leaves;
    left_child.push_back(0);
    right_child.push_back(0);
    split_feature.push_back(-1); // -1 means none
    threshold.push_back(0);
    leaf_value.push_back(leaf_value_);
    leaf_depth.push_back(depth);
}

int Tree::get_num_vertices() const {
    return num_leaves;
}

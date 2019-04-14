#include <utility>

#include "Tree.h"

Tree::Tree(const Tree& tree) {
    max_leaves = tree.max_leaves;
    num_leaves = tree.num_leaves;
    left_child = tree.left_child;
    right_child = tree.right_child;
    split_feature : tree.split_feature;
    threshold = tree.threshold;
    leaf_value = tree.leaf_value;
    leaf_depth = tree.leaf_depth;
}

Tree::Tree(int max_leaves) : max_leaves(max_leaves), num_leaves(0), left_child(2 * max_leaves),
right_child(2 * max_leaves), threshold(2 * max_leaves), leaf_value(2 * max_leaves),
leaf_depth(2 * max_leaves) {

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

    while (sfeature == split_feature[node]) {
        if (feature[sfeature] <= threshold[node]) {
            node = left_child[node];
        } else {
            node = right_child[node];
        }
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
    //split_feature.push_back(null);
    threshold.push_back(0);
    leaf_value.push_back(leaf_value_);
    leaf_depth.push_back(depth);
}

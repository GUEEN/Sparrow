#pragma once

#include <vector>
#include <utility>

#include "LabeledData.h"

struct TreeNode {
    int tree_index;
    int node_type;
    int feature;
    TFeature threshold;
    double left_predict;
    double right_predict;

    double gamma;
    double raw_martingale;
    double sum_c;
    double sum_c_squared;
    double bound;
    int num_scanned;
    bool fallback;
};

class Tree {
public:
    explicit Tree(int max_leaves);
    Tree(const Tree& tree);
    void release();

    std::pair<int, int> split(int leaf, int feature, int threshold,
        double left_value, double right_value);

    std::pair<int, double> get_leaf_index_prediction(const Example& data) const;
    double get_leaf_prediction(const Example& data) const;
    void add_new_node(double leaf_value, int depth);
    int get_num_leaves() const;

private:
    int max_leaves;
    int num_leaves;
    // left_child[i] is the left child of the node i
    std::vector<int> left_child;
    std::vector<int> right_child;
    std::vector<int> split_feature;
    std::vector<TFeature> threshold;
    std::vector<double> leaf_value;
    std::vector<int> leaf_depth;
};

typedef std::vector<Tree> Model;
typedef std::pair<Tree, double> TreeScore;
typedef std::pair<Model, double> ModelScore;


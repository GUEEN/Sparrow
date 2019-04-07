#pragma once

#include <vector>

struct Node {
    int feature_id;
    double threshold;

    int left;
    int right;
};

class Tree {

private:
    std::vector<Node> nodes;

};




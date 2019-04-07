#include "Learner.h"

Learner::Learner(int max_leaves,
    double min_gamma,
    double default_gamma,
    int num_examples_before_shrink) :
    max_leaves(max_leaves), min_gamma(min_gamma), default_gamma(default_gamma), 
    num_examples_before_shrink(num_examples_before_shrink) {

}
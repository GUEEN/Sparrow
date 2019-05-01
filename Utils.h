#pragma once

#include <vector>

#include "SerialStorage.h"

const double DELTA = 1e-6;
const double SHRINK = 1.0;
const double THRESHOLD_FACTOR = 1.0;
const double ALMOST_ZERO = 1e-8;

// Boosting related
double get_weight(const Example& data, double score);

std::vector<double> get_weights(const std::vector<Example>& data, const std::vector<double>& scores);

double get_bound(double sum_c, double sum_c_squared);

int get_sign(double a);

// Computational functions
bool is_zero(double a);

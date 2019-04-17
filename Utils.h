#pragma once

#include "SerialStorage.h"
#include "Tree.h"

const double DELTA = 1e-6;
const double SHRINK  = 1.0;
const double THRESHOLD_FACTOR = 1.0;
const double ALMOST_ZERO = 1e-8;

// Boosting related
inline double get_weight(const Example& data, double score) {
    // min(1.0, (-score * data.label).exp())
    return exp(-score * (data.label));
}

inline std::vector<double> get_weights(const std::vector<Example>& data, const std::vector<double>& scores) {
    double weight = 0;
    for (int i = 0; i < data.size(); ++i) {
        weight += get_weight(data[i], scores[i]);
    }
}

double get_bound(double sum_c, double sum_c_squared) {
    double threshold = THRESHOLD_FACTOR * 173.0 * log(4.0 / DELTA);

    if (sum_c_squared >= threshold) {
        double log_log_term = 3.0 * sum_c_squared / 2.0 / fabs(sum_c);
        double log_log = log_log_term > 2.7183 ? log(log(log_log_term)) : 0.0;
        return sqrt(SHRINK * (3.0 * sum_c_squared * (2.0 * log_log + log(2.0 / DELTA))));
    } else {
        return INFINITY;
    }
}

inline int get_sign(double a) {
    if (a < -ALMOST_ZERO) {
        return -1;
    } else {
        if (a > ALMOST_ZERO) {
            return 1;
        } else {
            return 0;
        }
    }
}
// Computational functions
bool is_zero(double a) {
    return get_sign(a) == 0;
}


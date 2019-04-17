#include <iostream>

#include "Bins.h"

Bins::Bins(int size, const DistinctValues& distinct_vals) {

    int avg_bin_size = distinct_vals.total_vals / size;

    double last_val = 0.0;
    int counter = 0;

    std::vector<double> vals;

    for (auto p: distinct_vals.distinct) {

        double k = p.first;
        int v = p.second;

        if (counter > avg_bin_size) {
            vals.push_back((last_val + k) / 2.0);
            counter = 0;
        }
        counter += v;
        last_val = k;
    }
    size = vals.size();
}

/// Return the number of thresholds. 
int Bins::len() const {
    return size;
}

/// Return the vector of thresholds.
std::vector<double>& Bins::get_vals() {
    return vals;
}

int Bins::get_split_index(double val) {

    if (vals.size() == 0 || val <= vals[0]) {
        return 0;
    }

    int left = 0;
    int right = size;

    while (left + 1 < right) {
        int mid = (left + right) / 2;
        if (val <= vals[mid]) {
            right = mid;
        } else {
            left = mid;
        }
    }
    return right;
}

int Bins::get_size() {
    return size;
}

void DistinctValues::update(double val) {
    ++total_vals;

    auto p = distinct.find(val);
    if (p == distinct.end()) {
        distinct[val] = 1;
    } else {
        (p->second)++;
    }
}

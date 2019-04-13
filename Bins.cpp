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

void DistinctValues::update(double val) {
    ++total_vals;

    auto p = distinct.find(val);
    if (p == distinct.end()) {
        distinct[val] = 1;
    } else {
        (p->second)++;
    }
}


std::vector<Bins> create_bins(
    int max_sample_size,
    int max_bin_size,
    Range range,
    SerialStorage& data_loader) {
    int start = range.start;
    int range_size = range.end - start;

    std::vector<DistinctValues> distinct(range_size);

    int remaining_reads = max_sample_size;

    while (remaining_reads > 0) {

    }

    std::vector<Bins> ret;
    
    for (const auto& mapper : distinct) {
        ret.emplace_back(max_bin_size, mapper);
    }
 
    std::cout << "Bins created" << std::endl;
    //exit(0);

    int total_bins = 0;
    for (const auto& t : ret) {
        total_bins += t.len();
    }    
    std::cout << "Bins are created. " << ret.size() << 
        " Features. " << total_bins << " Bins.";
    return ret;
}



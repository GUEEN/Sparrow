#pragma once

#include <map>
#include <vector>

// counter of distinct double values
struct DistinctValues {
    DistinctValues() = default;

    void update(double val);

    int total_vals;
    std::map<double, int> distinct;
};

class Bins {
public:
    Bins(int size, const DistinctValues& distinct_vals);

    int len() const;
    std::vector<double>& get_vals();

    int get_split_index(double val);
    int get_size();

private:
    int size;
    std::vector<double> vals;
};

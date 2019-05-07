#pragma once

#include <map>
#include <vector>

struct Range {
    int start;
    int end;
};

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
    int get_split_index(double val) const;

private:
    int size;
    std::vector<double> vals;
};

#include <map>
#include <vector>

#include "SerialStorage.h"

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

    int get_split_index(double val);

private:
    int size;
    std::vector<double> vals;
};

std::vector<Bins> create_bins(
    int max_sample_size,
    int max_bin_size,
    Range range,
    SerialStorage& data_loader
);




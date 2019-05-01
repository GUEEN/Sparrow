#pragma once

#include "DiskBuffer.h"
#include "Bins.h"

#include <vector>
#include <string>

class StratifiedStorage {
public:

    StratifiedStorage(
        int num_examples,
        int feature_size,
        const std::string& positive,
        int num_examples_per_block,
        const std::string& disk_buffer_filename
    );

    void init_stratified_from_file(
        const std::string& filename,
        int size,
        int batch_size,
        int feature_size,
        Range range,
        const std::vector<Bins>& bins);

private:

    std::string positive;

};

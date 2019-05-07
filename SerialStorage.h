#pragma once

#include <string>
#include <memory>
#include <tuple>
#include <vector>

#include "Io.h"
#include "Bins.h"
#include "LabeledData.h"

class TextToBinHelper;

class SerialStorage {
public:
    SerialStorage(
        const std::string& filename,
        int size,
        int feature_size,
        const std::string& positive,
        const std::vector<Bins>& bins,
        Range range
    );

    std::vector<RawExample> read_raw(int batch_size);
    std::vector<Example> read(int batch_size);

    void try_reset(bool force);

    void load_to_memory(int batch_size);


private:
    std::string filename;
    bool is_binary;
    bool in_memory;
    int size;
    int feature_size;
    std::string positive;

    int bytes_per_example;
        
    std::shared_ptr<TextToBinHelper> binary_cons;

    BufReader reader;
    std::vector<Example> memory_buffer;
    int index;
    std::vector<Bins> bins;
    Range range;

    int head;
    int tail;
};


class TextToBinHelper {
public:
    explicit TextToBinHelper(const std::string& original_filename);
    void append_data(const Example& data);

    std::tuple<std::string, int, int> get_content() const;

    std::string gen_filename();

private:
    std::string filename;
    int size;
    int bytes_per_example;
    BufWriter writer;
};

std::vector<Bins> create_bins(
    int max_sample_size,
    int max_bin_size,
    Range range,
    SerialStorage& data_loader
);


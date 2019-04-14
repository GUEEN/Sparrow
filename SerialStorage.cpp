#include <cassert>
#include <iostream>
#include <algorithm>

#include "SerialStorage.h"


SerialStorage::SerialStorage(
    const std::string& filename,
    int size,
    int feature_size,
    bool one_pass,
    const std::string& positive,
    const std::vector<Bins>& bins,
    Range range
    ) : filename(filename), size(size), feature_size(feature_size), is_binary(false), in_memory(false),
    positive(positive), bytes_per_example(0), index(0), range(range), bins(bins), head(0), tail(0),
    reader(filename) {
}

std::vector<RawExample> SerialStorage::read_raw(int batch_size) {
    head = index;
    tail = std::min(index + batch_size, size);
    int true_batch_size = tail - head;

    RawTFeature missing_value(0.0);
    auto batch = read_k_labeled_data<RawTFeature, TLabel>(reader, true_batch_size, missing_value, feature_size, positive);

    index = tail;
    try_reset(false /* not forcing */);
    return batch;
}

std::vector<Example> SerialStorage::read(int batch_size) {
    head = index;
    tail = std::min(index + batch_size, size);
    int true_batch_size = tail - head;

    // Load from memory
    if (in_memory) {
        return std::vector<Example>(memory_buffer.begin() + head, memory_buffer.begin() + tail);
    }
    // Load from disk
    int start = range.start;
    int end = range.end;

    std::vector<Example> batch;

    if (is_binary) {
        batch = read_k_labeled_data_from_binary_file(reader, true_batch_size, bytes_per_example);
    } else {
        std::vector<RawExample> raw_batch = 
            read_k_labeled_data<RawTFeature, TLabel>(reader, true_batch_size, 0, feature_size, positive);
    }

    if (is_binary) {
        read_k_labeled_data_from_binary_file(reader, true_batch_size, bytes_per_example);
    } else {
        std::vector<RawExample> raw_batch =
            read_k_labeled_data<RawTFeature, TLabel>(reader, true_batch_size, 0, feature_size, positive);

        for (const RawExample& rexample : raw_batch) {
            std::vector<TFeature> feature;
            for (int index = 0; index < rexample.feature.size(); ++index) {
                if (start <= index && index < end) {
                    double val = rexample.feature[index];
                    feature.push_back(bins[index - start].get_split_index(val));
                } else {
                    feature.push_back(0);
                }
            }
            batch.emplace_back(feature, rexample.label);
        }
    }

    //if let Some(ref mut cons) = self.binary_cons{
    //    batch.iter().for_each(| data | {
    //    cons.append_data(data);
    //});
    //}

    index = tail;
    try_reset(false);
    return batch;
}

void SerialStorage::try_reset(bool force) {
    if (index < size && !force ){
        return;
    }

    if (index >= size && binary_cons) {

        std::string orig_filename = filename;

        auto content = binary_cons->get_content();
        assert(size == std::get<1>(content));

        is_binary = true;
        filename = std::get<0>(content);
        bytes_per_example = std::get<2>(content);

        std::cout << "Text-based loader for " << orig_filename << " has been converted to Binary-based. \n Filename: "
            << filename << ", bytes_per_example: " << bytes_per_example << ".\n";
    }

    binary_cons = nullptr;
    index = 0;
    reader = create_bufreader(filename);
}

void SerialStorage::load_to_memory(int batch_size) {
    std::cout << "Load current file into the memory." << std::endl;
    assert(in_memory == false);

    try_reset(true);
    int num_batch = (size + batch_size - 1) / batch_size;

    for (int i = 0; i < num_batch; ++i) {
        std::vector<Example> data = read(batch_size);
        memory_buffer.insert(memory_buffer.end(), data.begin(), data.end());
    }
    
    in_memory = true;
    is_binary = true;
    std::cout << "In-memory conversion finished." << std::endl;
}


TextToBinHelper::TextToBinHelper(const std::string& original_filename) : size(0), bytes_per_example(0), writer(filename + "_bin") {
}

void TextToBinHelper::append_data(const Example& data) {

    int bytes_size = write_to_binary_file(writer, data);
    if (bytes_per_example > 0) {
        assert(bytes_per_example == bytes_size);
    }
    bytes_per_example = bytes_size;
    ++size;
}

std::tuple<std::string, int, int> TextToBinHelper::get_content() const {
    return std::make_tuple(filename, size, bytes_per_example);
}

std::string TextToBinHelper::gen_filename() {
    std::string random_str = "      ";
    for (int i = 0; i < 6; ++i) {
        random_str[i] = 'a' + rand() % 26;
    }
    return "data-" + random_str + ".bin";
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

        //let data = data_loader.read_raw(1000);
        //data.iter().for_each(| example | {
        //    let feature = &(example.feature);
        //    distinct.iter_mut()
        //        .enumerate()
        //        .for_each(| (idx, mapper) | {
        //        mapper.update(feature[start + idx] as f32);
        //    });
        //});
        //remaining_reads -= data.len();
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

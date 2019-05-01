#include "StratifiedStorage.h"

#include <iostream>

#include "SerialStorage.h"

StratifiedStorage::StratifiedStorage(
        int num_examples,
        int feature_size,
        const std::string& positive,
        int num_examples_per_block,
        const std::string& disk_buffer_filename
        ) : positive(positive) {
}

void StratifiedStorage::init_stratified_from_file(
    const std::string& filename,
    int size,
    int batch_size,
    int feature_size,
    Range range,
    const std::vector<Bins>& bins
) {
    SerialStorage reader(
        filename,
        size,
        feature_size,
        true,
        positive,
        std::vector<Bins>(),
        range
    );

    int index = 0;
    while (index < size) {
        std::vector<RawExample> data = reader.read_raw(batch_size);
        for (const RawExample& rexample : data) {
            const std::vector<RawTFeature>& raw_features = rexample.feature;
            std::vector<TFeature> features;
            for (int idx = 0; idx < raw_features.size(); ++idx) {
                RawTFeature val = raw_features[idx];
                if (range.start <= idx && idx < range.end) {
                    features.push_back(bins[idx - range.start].get_split_index(val));
                } else {
                    features.push_back(0);
                }
            }
            LabeledData<TFeature, TLabel> mapped_data(features, rexample.label);
            // send mapped data
        }
        index += batch_size;
    }

    std::cout << "Raw data on disk has been loaded into the stratified storage " << std::endl;
}


#pragma once

#include <vector>
#include <string>

#include "Bins.h"
#include "Channels.h"
#include "DiskBuffer.h"
#include "LabeledData.h"
#include "Tree.h"
#include "Utils.h"
#include "Strata.h"
#include "Samplers.h"


class StratifiedStorage {
public:
    StratifiedStorage(
        int num_examples,
        int feature_size,
        const std::string& positive,
        int num_examples_per_block,
        const std::string& disk_buffer_filename,
        int num_assigners,
        int num_samplers,
        Sender<std::pair<ExampleWithScore, int>>& sampled_examples,
        Receiver<Signal>& sampling_signal,
        Receiver<Model>& models,
        int channel_size,
        bool debug_mode
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
    std::pair<Sender<ExampleWithScore>, Receiver<ExampleWithScore>> updated_examples;
    std::shared_ptr<Strata> strata;
    std::pair<Sender< std::pair<int, std::pair<int, double>> >, Receiver< std::pair<int, std::pair<int, double>> >> stats_update;
    WeightTableRead weights_table_r;

};

int sample_weights_table(WeightTableRead& weight_table_r);

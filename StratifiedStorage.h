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
        //Receiver<Signal>& sampling_signal,
        std::shared_ptr<Model>& model,
        int channel_size,
        bool debug_mode
    );

    void init_stratified_from_file(
        const std::string& filename,
        int size,
        int batch_size,
        int feature_size,
        const std::vector<Bins>& bins);

private:

    std::string positive;
    std::pair<Sender<ExampleWithScore>, Receiver<ExampleWithScore>> updated_examples;
    std::shared_ptr<Strata> strata;
    std::pair<Sender< std::pair<int, std::pair<int, double>> >, Receiver< std::pair<int, std::pair<int, double>> >> stats_update;
    WeightsTable weights_table;
    std::shared_ptr<Samplers> samplers;
    std::shared_ptr<Model> model;

};

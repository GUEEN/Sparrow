#pragma once

#include <map>
#include <atomic>

#include "Channels.h"
#include "Tree.h"
#include "Strata.h"

// !!!! temporary
typedef std::map<int, double> WeightTableRead;

class Samplers {
public:
    Samplers(
        std::shared_ptr<Strata>& strata,
        Sender<std::pair<ExampleWithScore, int>>& sampled_examples,
        Sender<ExampleWithScore>& updated_examples,
        std::shared_ptr<Model>& model,
        Sender<std::pair<int, std::pair<int, double>>>& stats_update_s,
        WeightTableRead& weights_table,
        // sampling_signal_channel: Receiver<Signal>,
        int num_threads);

    void run();

private:

    std::shared_ptr<Strata>& strata;
    Sender<std::pair<ExampleWithScore, int>>& sampled_examples;
    Sender<ExampleWithScore>& updated_examples;
    std::shared_ptr<Model> model;
    Signal sampling_signal;
    Sender<std::pair<int, std::pair<int, double>>>& stats_update_s;
    WeightTableRead& weights_table;
    // sampling_signal_channel: Receiver<Signal>,
    int num_threads;
};

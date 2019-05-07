#pragma once

#include <map>
#include <atomic>

#include "Channels.h"
#include "Tree.h"
#include "Strata.h"

class WeightsTable {
public:
    void incr(int key, double value) {
        if (value == 0)
            return;
        std::lock_guard<std::mutex> lock(mutex);
        data[key] += value;
    }

    std::vector<std::pair<int, double>> get_data() const {
        // TODO: create a copy iff there were writes in between?
        std::lock_guard<std::mutex> lock(mutex);
        return std::vector<std::pair<int, double>>(data.begin(), data.end());
    };

private:
    mutable std::mutex mutex;
    std::map<int, double> data;
};

class Samplers {
public:
    Samplers(
        std::shared_ptr<Strata>& strata,
        Sender<std::pair<ExampleWithScore, int>>& sampled_examples,
        Sender<ExampleWithScore>& updated_examples,
        std::shared_ptr<Model>& model,
        Sender<std::pair<int, std::pair<int, double>>>& stats_update_s,
        const WeightsTable& weights_table,
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
    const WeightsTable& weights_table;
    // sampling_signal_channel: Receiver<Signal>,
    int num_threads;
};

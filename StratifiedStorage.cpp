#include "StratifiedStorage.h"

#include <cmath>
#include <iostream>
#include <map>
#include <thread>

#include "Channels.h"
#include "SerialStorage.h"
#include "ThreadManager.h"
#include "Utils.h"

using std::thread;
using std::pair;

void weight_table_thread(WeightsTable& weights_table, Receiver<pair<int, pair<int, double>>>& stats_update_r) {
    while (ThreadManager::continue_run) {
        auto recv = stats_update_r.try_recv();
        if (!recv.first) {
            std::this_thread::yield();
            continue;
        }

        const auto& p = recv.second;
        int index = p.first;
        double weight = p.second.second;
        weights_table.incr(index, weight);
    }
}

void assigner_thread(
    Receiver<ExampleWithScore>& updated_examples_r,
    std::shared_ptr<Strata>& strata,
    Sender<pair<int, pair<int, double>>>& stats_update_s) {

    while (ThreadManager::continue_run) {
        auto ret = updated_examples_r.try_recv();
        if (!ret.first) {
            //break;
            std::this_thread::yield();
            continue;
        }

        const ExampleWithScore& ret_value = ret.second;
        const Example& example = ret_value.first;
        double score = ret_value.second.first;
        int version = ret_value.second.second;

        // !!! stuff from oberon.  Rust code was: let index = "weight.log2() as i8".
        float weight = get_weight(example, score);
        int index;
        frexp(weight, &index);

        strata->send(index, example, score, version);
        stats_update_s.send(std::make_pair(index, std::make_pair(1, static_cast<double>(weight))));
    }
}

void launch_assigner_threads(
    std::shared_ptr<Strata>& strata,
    Receiver<ExampleWithScore>& updated_examples_r,
    Sender<pair<int, pair<int, double>>>& stats_update_s,
    int num_threads) {
    for (int i = 0; i < num_threads; i++) {
        std::cout << "Launch assigner thread " << i << std::endl;
        thread th(assigner_thread, std::ref(updated_examples_r), std::ref(strata), std::ref(stats_update_s));
        th.detach();
    }
}


StratifiedStorage::StratifiedStorage(
    int num_examples,
    int feature_size,
    const std::string& positive,
    int num_examples_per_block,
    const std::string& disk_buffer_filename,
    int num_assigners,
    int num_samplers,
    Sender<std::pair<ExampleWithScore, int>>& sampled_examples,
    std::shared_ptr<Model>& model,
    //Receiver<Signal>& sampling_signal,
    int channel_size,
    bool debug_mode) : positive(positive),
    updated_examples(bounded_channel<ExampleWithScore>(channel_size, "updated-examples")),
    strata(new Strata(num_examples, feature_size, num_examples_per_block, disk_buffer_filename)),
    stats_update(bounded_channel<pair<int, pair<int, double>>>(5000000, "stats")),
    model(model)
 {
    std::cout << "debug_mode=" << debug_mode << std::endl;

    std::thread thw(weight_table_thread, std::ref(weights_table), std::ref(stats_update.second));
    thw.detach();

    launch_assigner_threads(strata, updated_examples.second, stats_update.first, num_assigners);

    samplers.reset(new Samplers(strata,
        sampled_examples,
        updated_examples.first,
        model,
        stats_update.first,
        weights_table,
        // sampling_signal,
        num_samplers));

    samplers->run();

    // this->counts_table_r = counts_table_r;
    // this->weights_table_r = weights_table_r;
    // this->updated_examples_s = updated_examples_s;
}

void StratifiedStorage::init_stratified_from_file(
    const std::string& filename,
    int size,
    int batch_size,
    int feature_size,
    const std::vector<Bins>& bins) {
    SerialStorage reader(
        filename,
        size,
        feature_size,
        positive,
        std::vector<Bins>()
    );

    int index = 0;
    while (index < size) {
        std::vector<RawExample> data = reader.read_raw(batch_size);
        for (const RawExample& rexample : data) {
            const std::vector<RawTFeature>& raw_features = rexample.feature;
            std::vector<TFeature> features;
            for (int idx = 0; idx < raw_features.size(); ++idx) {
                RawTFeature val = raw_features[idx];
                features.push_back(bins[idx].get_split_index(val));
            }
            LabeledData<TFeature, TLabel> mapped_data(features, rexample.label);
            updated_examples.first.send(std::make_pair(mapped_data, std::make_pair(0.0, 0)));
        }
        index += batch_size;
    }

    std::cout << "Raw data on disk has been loaded into the stratified storage " << std::endl;
}

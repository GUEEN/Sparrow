#include "StratifiedStorage.h"

#include <cmath>
#include <iostream>
#include <map>
#include <thread>

#include "Channels.h"
#include "SerialStorage.h"
#include "Utils.h"

using std::thread;
using std::pair;

void assigner_thread(
    Receiver<ExampleWithScore>& updated_examples_r,
    std::shared_ptr<Strata>& strata,
    Sender<pair<int, pair<int, double>>>& stats_update_s) {
    while (true) {
        auto ret = updated_examples_r.try_recv();
        if (!ret.first) {
            //break;
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
    Receiver<Signal>& sampling_signal,
    Receiver<Model>& models,
    int channel_size,
    bool debug_mode) : positive(positive),
    updated_examples(bounded_channel<ExampleWithScore>(channel_size, "updated-examples")),
    strata(new Strata(num_examples, feature_size, num_examples_per_block, disk_buffer_filename)),
    stats_update(bounded_channel<pair<int, pair<int, double>>>(5000000, "stats"))
{
    std::cout << "debug_mode=" << debug_mode << std::endl;


    launch_assigner_threads(strata, updated_examples.second, stats_update.first, num_assigners);

    std::shared_ptr<Samplers> samplers(new Samplers(
        strata,
        sampled_examples,
        updated_examples.first,
        models,
        stats_update.first,
        weights_table_r,
       // sampling_signal,
        num_samplers
    ));

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
    Range range,
    const std::vector<Bins>& bins) {
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
            updated_examples.first.send(std::make_pair(mapped_data, std::make_pair(0.0, 0)));
        }
        index += batch_size;
    }

    std::cout << "Raw data on disk has been loaded into the stratified storage " << std::endl;
}

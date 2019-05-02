#include "StratifiedStorage.h"

#include <cmath>
#include <iostream>
#include <map>
#include <thread>

#include "Channels.h"
#include "SerialStorage.h"
#include "Strata.h"
#include "Utils.h"

using std::thread;
using std::pair;

int sample_weights_table(WeightTableRead& weights_table_r) {

    double sum_of_weights = 0.0;
    for (const auto& p : weights_table_r) {
        sum_of_weights += p.second;
    }

    if (get_sign(sum_of_weights) == 0) {
        return -1;
    } else {
        double frac = (static_cast<double>(rand()) / (RAND_MAX)) * sum_of_weights;
        auto iter = weights_table_r.begin();

        std::pair<int, double> key_val(0, 0.0);
        
        while (get_sign(frac) >= 0) {
            key_val = *iter;
            frac -= key_val.second;
            ++iter;
        }
        return key_val.first;
    }
}


void assigner_thread(
    Receiver<ExampleWithScore>& updated_examples_r,
    Strata& strata,
    Sender<pair<int, pair<int, double>>>& stats_update_s) {
    while (true) {
        auto ret = updated_examples_r.try_recv();
        if (!ret.first) {
            break;
        }

        const ExampleWithScore& ret_value = ret.second;
        const Example& example = ret_value.first;
        double score = ret_value.second.first;
        int version = ret_value.second.second;

        // !!! stuff from oberon.  Rust code was: let index = "weight.log2() as i8".
        float weight = get_weight(example, score);
        int index;
        frexp(weight, &index);

        strata.send(index, example, score, version);
        stats_update_s.send(std::make_pair(index, std::make_pair( 1, static_cast<double>(weight) ) ));
    }
}

void sampler_thread(Strata& strata,
    Sender<pair<ExampleWithScore, int>>& sampled_examples,
    Sender<ExampleWithScore>& updated_examples,
    Model model,
    Sender<pair<int, pair<int, double>>>& stats_update_s,
    WeightTableRead& weights_table
    ) {

    std::map<int, double> grids;

    int num_updated = 0;
    int num_sampled = 0;

    while (true) {
        // STEP 1: Sample which strata to get next sample
        int index = sample_weights_table(weights_table);
        if (index == -1) { // index is none
            // stratified storage is empty, wait for data loading
            std::cerr << "Sampler sleeps waiting for data loading" << std::endl;
            //sleep(Duration::from_millis(1000));
            continue;
        }

        // STEP 2: Access the queue for the sampled strata
        std::unique_ptr<OutQueueReceiver>& receiver = strata.get_out_queue(index);


        // STEP 3: Sample one example using minimum variance sampling
        // meanwhile update the weights of all accessed examples
        double grid_size = static_cast<double>(1 << (index + 1));

        //if (SPEED_TEST) {
        //    grid_size = 55.0 * 10.0;
        //}

        if (grids.find(index) == grids.end()) {
            grids[index] = (static_cast<double>(rand()) / RAND_MAX) * grid_size;
        }

        double& grid = grids[index];

        pair<bool, ExampleWithScore> sampled_example(false, ExampleWithScore());

        while (sampled_example.first == false) {

            int failed_recv = 0;
            pair<bool, ExampleWithScore> rcv(false, ExampleWithScore());

            while (rcv.first == false && failed_recv < 5) {
                rcv = receiver->try_recv();
                failed_recv++;
            }
            
            if (rcv.first == false) {
                break;
            }

            ExampleWithScore& exsc = rcv.second;

            Example example = exsc.first;
            double score = exsc.second.first;
            int version = exsc.second.second;

            double updated_score = score;
            int model_size = model.size();

            for (int index = version; index < model_size; ++index) {
                updated_score += model[index].get_leaf_prediction(example);
            }

            grid += get_weight(example, updated_score);

            if (grid >= grid_size) {
                sampled_example = std::make_pair(true, std::make_pair(example, std::make_pair(updated_score, model_size) ));
            }

            stats_update_s.send(std::make_pair(index, std::make_pair(-1, -get_weight(example, score)) ));
            updated_examples.send(std::make_pair(example, std::make_pair(updated_score, model_size)));
            num_updated ++;
        }

        // STEP 4: Send the sampled example to the buffer loader
        if (sampled_example.first == false) {
            std::cerr << "Sampling the stratum " << index << " failed because it has too few examples" << std::endl;
            continue;
        }
        
        int sample_count = static_cast<int>(grid / grid_size);

        if (sample_count > 0) {
            sampled_examples.send(std::make_pair(sampled_example.second, sample_count));
            grid -= grid_size * static_cast<double>(sample_count);
            num_sampled += sample_count;
        }

    }
}


void launch_assigner_threads(
    Strata& strata,
    Receiver<ExampleWithScore>& updated_examples_r,
    Sender<pair<int, pair<int, double>>>& stats_update_s,
    int num_threads) {
    for (int i = 0; i < num_threads; i++) {
        std::cout << "Launch assigner thread " << i << std::endl;
        thread th(assigner_thread, std::ref(updated_examples_r), std::ref(strata), std::ref(stats_update_s));
        th.detach();
    }
}

void launch_sampler_threads(
    Strata& strata,
    Sender<pair<ExampleWithScore, int>>& sampled_examples,
    Sender<ExampleWithScore>& updated_examples,
    Receiver<Model>& next_model,
    Sender<pair<int, pair<int, double>>>& stats_update_s,
    WeightTableRead& weights_table,
    Receiver<Signal>& sampling_signal,
    int num_threads) {

    Model model = next_model.recv();
    std::cout << "Sampler model update" << std::endl;

    for (int i = 0; i < num_threads; i++) {
        std::cout << "Launch sampler thread " << i << std::endl;
        thread th(sampler_thread, std::ref(strata), std::ref(sampled_examples), 
            std::ref(updated_examples), model,
            std::ref(stats_update_s), std::ref(weights_table));
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
    updated_examples(bounded_channel<ExampleWithScore>(channel_size, "updated-examples")) {
    std::cerr << "debug_mode=" << debug_mode << std::endl;

    WeightTableRead weights_table_r;
    Strata strata(num_examples, feature_size, num_examples_per_block, disk_buffer_filename);

    auto stats_update = bounded_channel<pair<int, pair<int, double>>>(5000000, "stats");

    launch_assigner_threads(strata, updated_examples.second, stats_update.first, num_assigners);
    launch_sampler_threads(strata, sampled_examples, updated_examples.first, models,
        stats_update.first, weights_table_r, sampling_signal, num_samplers);

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
            updated_examples.first.send(std::make_pair(mapped_data, std::make_pair( 0.0, 0 )));
        }
        index += batch_size;
    }

    std::cout << "Raw data on disk has been loaded into the stratified storage " << std::endl;
}

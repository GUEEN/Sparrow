#include "Samplers.h"
#include "Utils.h"
#include "ThreadManager.h"

#include <thread>
#include <iostream>
#include <chrono>

int sample_weights_table(WeightTableRead weights_table_r) {
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

        while (get_sign(frac) >= 0 && iter != weights_table_r.end()) {
            key_val = *iter;
            frac -= key_val.second;
            ++iter;
        }
        return key_val.first;
    }
}

void sampler_model_thread(
    Receiver<Model>& next_model,
    std::shared_ptr<Model>& model
) {
    std::mutex mutex;
    std::thread::id id = std::this_thread::get_id();

    while (ThreadManager::continue_run(id)) {

        Model new_model = next_model.recv();

        std::lock_guard<std::mutex> lock(mutex);

        model.reset(new Model(next_model.recv()));
        std::cout << "Sampler model update" << std::endl;
    }

    ThreadManager::done(id);
}


void sampler_thread(
    std::shared_ptr<Strata>& strata,
    Sender<std::pair<ExampleWithScore, int>>& sampled_examples,
    Sender<ExampleWithScore>& updated_examples,
    std::shared_ptr<Model>& model,
    Sender<std::pair<int, std::pair<int, double>>>& stats_update_s,
    WeightTableRead& weights_table
) {

    std::map<int, double> grids;

    int num_updated = 0;
    int num_sampled = 0;

    std::thread::id id = std::this_thread::get_id();
    while (ThreadManager::continue_run(id)) {
        // STEP 1: Sample which strata to get next sample
        int index = sample_weights_table(weights_table);
        if (index == -1) { // index is none
            // stratified storage is empty, wait for data loading
            //std::cout << "Sampler sleeps waiting for data loading" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }

        // STEP 2: Access the queue for the sampled strata
        std::unique_ptr<OutQueueReceiver>& receiver = strata->get_out_queue(index);


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

        std::pair<bool, ExampleWithScore> sampled_example(false, ExampleWithScore());

        while (sampled_example.first == false && ThreadManager::continue_run(id)) {

            int failed_recv = 0;
            std::pair<bool, ExampleWithScore> rcv(false, ExampleWithScore());

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
            int model_size = model->size();

            for (int index = version; index < model_size; ++index) {
                updated_score += model->at(index).get_leaf_prediction(example);
            }

            grid += get_weight(example, updated_score);

            if (grid >= grid_size) {
                sampled_example = std::make_pair(true, std::make_pair(example, std::make_pair(updated_score, model_size)));
            }

            stats_update_s.send(std::make_pair(index, std::make_pair(-1, -get_weight(example, score))));
            updated_examples.send(std::make_pair(example, std::make_pair(updated_score, model_size)));
            num_updated++;
        }

        // STEP 4: Send the sampled example to the buffer loader
        if (sampled_example.first == false) {
            //std::cout << "Sampling the stratum " << index << " failed because it has too few examples" << std::endl;
            continue;
        }

        int sample_count = static_cast<int>(grid / grid_size);

        if (sample_count > 0) {
            sampled_examples.send(std::make_pair(sampled_example.second, sample_count));
            grid -= grid_size * static_cast<double>(sample_count);
            num_sampled += sample_count;
        }
    }

    ThreadManager::done(id);
}


Samplers::Samplers(
    std::shared_ptr<Strata>& strata,
    Sender<std::pair<ExampleWithScore, int>>& sampled_examples,
    Sender<ExampleWithScore>& updated_examples,
    Receiver<Model>& next_model,
    Sender<std::pair<int, std::pair<int, double>>>& stats_update_s,
    WeightTableRead& weights_table,
    // sampling_signal_channel: Receiver<Signal>,
    int num_threads) : strata(strata), sampled_examples(sampled_examples), updated_examples(updated_examples),
    next_model(next_model), model(new Model()), sampling_signal(Signal::STOP), stats_update_s(stats_update_s),
    weights_table(weights_table), num_threads(num_threads) {}
    // sampling_signal_channel: sampling_signal_channel,

void Samplers::run() {

    std::thread thm(sampler_model_thread, std::ref(next_model), std::ref(model));
    ThreadManager::add(thm.get_id());
    thm.detach();
    
    //Signal signal = sampling_signal;
    //*signal = Signal::START;  // new_signal;
    //*signal == Signal::START

    for (int i = 0; i < num_threads; i++) {
        std::cout << "Launch sampler thread " << i << std::endl;
        std::thread th(sampler_thread,
            std::ref(strata), std::ref(sampled_examples),
            std::ref(updated_examples), std::ref(model),
            std::ref(stats_update_s), std::ref(weights_table));

        ThreadManager::add(th.get_id());
        th.detach();
    }
    
}

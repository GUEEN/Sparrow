// Sparrow.cpp : Defines the entry point for the console application.
//

#include <string>
#include <iostream>

#include "Config.h"
#include "Boosting.h"
#include "SerialStorage.h"
#include "StratifiedStorage.h"

void validate(std::shared_ptr<Model>& model, const Config& config, std::vector<Bins>& bins) {

    int num_examples = config.num_testing_examples;

    SerialStorage data = SerialStorage(
        config.testing_filename,
        num_examples,
        config.num_features,
        config.positive,
        bins
    );

    std::vector<double> scores(num_examples);
    std::vector<TLabel> labels(num_examples);

    int index = 0;
    while (index < num_examples) {
        std::vector<Example> batch = data.read(config.batch_size);
        for (int k = 0; k < model->size(); ++k) {
            const Tree& tree = (*model)[k];
            for (int i = 0; i < batch.size() && i + index < num_examples; ++i) {

                Example& example = batch[i];
                scores[index + i] += tree.get_leaf_prediction(example);
            }
        }
        for (int i = 0; i < batch.size() && i + index < num_examples; ++i) {

            Example& example = batch[i];
            labels[index + i] = example.label;
        }
        index += batch.size();
    }

    // output

    int corr = 0;
    int npos = 0;
    int nneg = 0;

    for (int i = 0; i < num_examples; ++i) {
        if (get_sign(scores[i]) == labels[i]) {
            ++corr;
        }
        if (labels[i] == 1) {
            npos++;
        } else {
            nneg++;
        }

    }

    std::cout << "There are " << num_examples << " testing examples" << std::endl;
    std::cout << "The model correctly predicts " << corr << " of them" << std::endl;
    
}

// takes trees from a given forest and reweights them according to AdaBoost
std::shared_ptr<Model> AdaBoost(const Config& config, std::shared_ptr<Model> initial_model, 
    const std::vector<Bins>& bins, int max_iterations) {
    int num_examples = config.num_examples;

    SerialStorage data = SerialStorage(
        config.testing_filename,
        num_examples,
        config.num_features,
        config.positive,
        bins
    );

    // we read all example at once. Run this on small files
    std::vector<Example> examples = data.read(num_examples);

    Model new_model(*initial_model);

    int trees = initial_model->size();

    std::vector<double> d(num_examples, 1.0 / num_examples);
    std::vector<double> weights(num_examples);
    std::vector<double> error(trees);

    for (int iter = 0; iter < max_iterations; ++iter) {
        // compute erorrs;
        error.assign(trees, 0.0);
        for (int i = 0; i < trees; ++i) {
            error[i] = 0.0;
            for (int j = 0; j < num_examples; ++j) {
                double pred = new_model[i].get_leaf_prediction(examples[j]);
                if (get_sign(pred) != examples[j].label) {
                    error[i] += d[j];
                }
            }
        }

        int i0 = 0;
        for (int i = 1; i < trees; ++i) {
            if (error[i] < error[i0]) {
                i0 = i;
            }
        }

        if (error[i0] > 0.5) {
            break;
        }

        double alpha = 0.5 * log((1.0 - error[i0]) / error[i0]);
        //std::cout << "Max error is " << error[i0] << " Add weight " << alpha << " to classifier " << i0 << std::endl;
        weights[i0] += alpha;

        // recompute the distribution
        double d_sum = 0.0;
        for (int j = 0; j < num_examples; ++j) {
            double pred = new_model[i0].get_leaf_prediction(examples[j]);
            d[j] *= exp(-alpha * examples[j].label * get_sign(pred));
            d_sum += d[j];
        }
        for (int j = 0; j < num_examples; ++j) {
            d[j] /= d_sum;
        }        
    }

    for (int i = 0; i < trees; ++i) {
        new_model[i].set_weight(weights[i]);
    }

    return std::shared_ptr<Model>(new Model(new_model));
}

void training(const Config& config) {

    // Strata -> BufferLoader
    std::pair<Sender<std::pair<ExampleWithScore, int>>, Receiver<std::pair<ExampleWithScore, int>>> examples_channel
        = bounded_channel<std::pair<ExampleWithScore, int>>(config.channel_size, "gather-samples");

    // BufferLoader -> Strata
    std::pair<Sender<Signal>, Receiver<Signal>> signal_channel =
        bounded_channel<Signal>(10, "sampling-signal");
   
    std::cout << "Creating bins" << std::endl;

    SerialStorage serial_training_loader(
        config.training_filename,
        config.num_examples,
        config.num_features,
        config.positive,
        std::vector<Bins>()
        );

    std::vector<Bins> bins = create_bins(
        config.num_examples, config.max_bin_size, config.num_features, serial_training_loader);
    
    std::vector<Example> validation_set1;
    std::vector<Example> validation_set2;

    std::shared_ptr<Model> model(new Model());

    std::cout << "Starting the stratified structure." << std::endl;
    StratifiedStorage stratified_structure(
        config.num_examples,
        config.num_features,
        config.positive,
        config.num_examples_per_block,
        config.disk_buffer_filename,
        config.num_assigners,
        config.num_samplers,
        examples_channel.first,
        //signal_channel.second,
        model,
        config.channel_size,
        config.debug_mode
        );
    std::cout << "Initializing the stratified structure." << std::endl;
    stratified_structure.init_stratified_from_file(
        config.training_filename,
        config.num_examples,
        config.batch_size,
        config.num_features,
        bins
        );


    std::cout << "Starting the buffered loader" << std::endl;
    BufferLoader buffer_loader(
        config.buffer_size,
        config.batch_size,
        examples_channel.second,
        signal_channel.first,
        config.serial_sampling,
        true,
        config.min_ess);

    std::cout << "Starting the booster" << std::endl;
    Boosting booster(
        config.num_iterations,
        config.max_leaves,
        config.min_gamma,
        config.max_trials_before_shrink,
        bins,
        buffer_loader,
        // serial_training_loader,
        config.max_sample_size,
        config.default_gamma,
        model
        );


    std::cout << "Start training" << std::endl;
    booster.training(validation_set1, validation_set2);
    std::cout << "Training complete" << std::endl;

    std::cout << "Testing our model" << std::endl;

    validate(model, config, bins);

    // recompute weights according to adaboost for comparison
    std::cout << "Reweight the trees with AdaBoost" << std::endl;
    std::shared_ptr<Model> ab_model = AdaBoost(config, model, bins, 1000);
    validate(ab_model, config, bins);

}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Incorrect number of arguments: " << argc << ", expected 2" << std::endl;
        return 1;
    }
    
    std::string path = argv[1];
    Config config = read_config(path);
    training(config);

    return 0;
}

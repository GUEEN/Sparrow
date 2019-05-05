// Sparrow.cpp : Defines the entry point for the console application.
//

#include <string>
#include <iostream>

#include "Boosting.h"
#include "SerialStorage.h"
#include "StratifiedStorage.h"

struct Config {
    /// File path to the training data
    std::string training_filename;
    /// Number of training examples
    int num_examples;
    /// Number of features
    int num_features;
    /// Range of the features for creating weak rules
    Range range;
    /// Label for positive examples
    std::string positive;
    /// File path to the testing data
    std::string testing_filename;
    /// Number of testing examples
    int num_testing_examples;
    /// Number of examples to scan for generating heuristic used in Sparrow
    int max_sample_size;
    /// Maximum number of bins for discretizing continous feature values
    int max_bin_size;
    /// Minimum value of the gamma of the generated tree nodes
    double min_gamma;
    /// Default maximum value of the \gamma for generating tree nodes
    double default_gamma;
    /// Maximum number of examples to scan before shrinking the value of \gamma
    int max_trials_before_shrink;
    /// Minimum effective sample size for triggering resample
    double min_ess;
    /// Number of boosting iterations
    int num_iterations;
    /// Maximum number of tree leaves in each boosted tree
    int max_leaves;
    /// Maximum number of elements in the channel connecting scanner and sampler
    int channel_size;
    /// Number of examples in the sample set that needs to be loaded into memory
    int buffer_size;
    /// Number of examples to process in each weak rule updates
    int batch_size;
    /// Set to true to stop running sampler in the background of the scanner
    bool serial_sampling;
    /// Number of examples in a block on the stratified binary file
    int num_examples_per_block;
    /// File name for the stratified binary file
    std::string disk_buffer_filename;
    /// Number of threads for putting examples back to correct strata
    int num_assigners;
    /// Number of threads for sampling examples from strata
    int num_samplers;
    /// Flag for keeping all intermediate models during training (for debugging purpose)
    bool save_process;
    /// Number of iterations between persisting models on disk
    int save_interval;
    /// Flag for activating debug mode
    bool debug_mode;
    /// (for validation only) the file names of the models to run the validation
    std::string models_table_filename;
    /// Flag indicating if models are trained incrementally
    bool incremental_testing;
    /// Flag for validation mode, set to true to output raw scores of testing examples,
    /// and set to false for printing the validation scores but not raw scores
    bool testing_scores_only;
};

Config a1a_config = {
    "C:\\DATA\\LIBSVM\\a1a",
    30956,
    125,
    {0, 125},
    "+1",
    "C:\\DATA\\LIBSVM\\a1a.t",
    30956,
    20000,
    10,
    0.00001,
    0.25,
    20000,
    0.5,
    4, //64,
    2,
    128,
    20000,
    1000,
    false,
    128,
    "stratified.bin",
    2,
    2,
    false,
    1,
    false,
    "models_table.txt",
    true,
    false
};

void validate(const Model& model, const Config& config, std::vector<Bins>& bins) {

    int num_examples = config.num_testing_examples;

    SerialStorage data = SerialStorage(
        config.testing_filename,
        num_examples,
        config.num_features,
        false,
        config.positive,
        bins,
        { 0, config.num_features }
    );

    std::vector<double> scores(num_examples);
    std::vector<TLabel> labels(num_examples);

    int index = 0;
    while (index < num_examples) {
        std::vector<Example> batch = data.read(config.batch_size);
        for (int k = 0; k < model.size(); ++k) {
            Tree tree = model[k];
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

    std::cout << "We have " << num_examples << "testing examples" << std::endl;
    std::cout << "The model correctly predicts" << corr << " of them" << std::endl;
    
}

void training(const Config& config) {

    // Strata -> BufferLoader
    std::pair<Sender<std::pair<ExampleWithScore, int>>, Receiver<std::pair<ExampleWithScore, int>>> examples_channel
        = bounded_channel<std::pair<ExampleWithScore, int>>(config.channel_size, "gather-samples");

    // BufferLoader -> Strata
    std::pair<Sender<Signal>, Receiver<Signal>> signal_channel =
        bounded_channel<Signal>(10, "sampling-signal");

    // Booster -> Strata
    std::pair<Sender<Model>, Receiver<Model>> model_channel =
        bounded_channel<Model>(config.channel_size, "updated-models");
    
    std::cout << "Creating bins" << std::endl;

    SerialStorage serial_training_loader(
        config.training_filename,
        config.num_examples,
        config.num_features,
        true,
        config.positive,
        std::vector<Bins>(),
        config.range
        );

    std::vector<Bins> bins = create_bins(
        config.max_sample_size, config.max_bin_size, config.range, serial_training_loader);

    std::vector<Example> validation_set1;
    std::vector<Example> validation_set2;

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
        signal_channel.second,
        model_channel.second,
        config.channel_size,
        config.debug_mode
        );
    std::cout << "Initializing the stratified structure." << std::endl;
    stratified_structure.init_stratified_from_file(
        config.training_filename,
        config.num_examples,
        config.batch_size,
        config.num_features,
        config.range,
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
        config.range,
        config.max_sample_size,
        config.default_gamma,
        model_channel.first
        );


    std::cout << "Start training" << std::endl;
    booster.training(validation_set1, validation_set2);
    std::cout << "Training complete" << std::endl;

    std::cout << "Testing our model" << std::endl;

    Model model = booster.get_model();

    validate(model, config, bins);

}


void testing(const Config& config) {
    // Load configurations
  /*  validate(
        config.models_table_filename.clone(),
        config.testing_filename.clone(),
        config.num_testing_examples,
        config.num_features,
        config.batch_size,
        config.positive.clone(),
        config.incremental_testing,
        config.testing_scores_only,
        );*/
}



int main() {

    training(a1a_config);


    return 0;
}

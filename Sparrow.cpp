// Sparrow.cpp : Defines the entry point for the console application.
//

#include "Boosting.h"
#include "SerialStorage.h"

#include <string>
#include <iostream>

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
    64,
    2,
    128,
    20000,
    1000,
    false,
    128,
    "stratified.bin",
    false,
    1,
    false,
    "models_table.txt",
    true,
    false
};

void validate(
    std::string models_table,
    std::string testing_filename,
    int num_examples,
    int num_features,
    int batch_size,
    std::string positive,
    bool incremental_testing
    ) {
    // TODO: make eval_funcs a parameter
    //let eval_funcs = vec![EvalFunc::AdaBoostLoss, EvalFunc::AUPRC, EvalFunc::AUROC];
    //let bins = serde_json::from_str(&read_all(&"models/bins.json".to_string()))
    //    .expect(&format!("Cannot parse the bins in `{}`", "models/bins.json"));

    // recover bins from file 'models/bins.json'
    std::vector<Bins> bins;

    BufReader models_list = create_bufreader(models_table);

    SerialStorage data = SerialStorage(
        testing_filename,
        num_examples,
        num_features,
        false,
        positive,
        bins,
        { 0, num_features }
        );

    std::vector<double> scores(num_examples);
    std::vector<TLabel> labels(num_examples);

    int last_model_length = 0;

    for (;;) {
        //if models_list.read_line(&mut line).is_err() || line.trim() == "" {
        //    break;
        //}
        std::string line;
        models_list.read_line(line);
        if (line == "") {
            break;
        }

        //let filepath = line.to_string().trim().to_string();
        std::string filepath = line;

        //// validate model
        //let(ts, _, model) : (f32, usize, Model) =
        //    serde_json::from_str(&read_all(&filepath))
        //    .expect(&format!("Cannot parse the model in `{}`", filepath));

        // recover bins and model from filepath
        Model model;

        int index = 0;
        while (index < num_examples) {
            std::vector<Example> batch = data.read(batch_size);
            for (int k = last_model_length; k < model.size(); ++k) {
                Tree tree = model[k];
                for (int i = 0; i < batch.size(); ++i) {

                    Example& example = batch[i];
                    scores[index + i] += tree.get_leaf_prediction(example);
                }
            }
            for (int i = 0; i < batch.size(); ++i) {

                Example& example = batch[i];
                labels[index + i] += example.label;
            }
            index += batch.size();
        }

        // output
        std::string outputpath = filepath + "_scores";

        std::vector<std::string> preds;
        for (double score : scores) {
            preds.push_back(std::to_string(score));
        }

        std::string content = preds[0];
        for (int i = 1; i < preds.size(); ++i) {
            content = content + "\n" + preds[i];
        }

        write_all(outputpath, content);
        std::cout << "Processed " << filepath << std::endl;

        // Reset scores if necessary
        if (incremental_testing) {
            last_model_length = model.size();
        } else {
            for (int i = 0; i < scores.size(); ++i) {
                scores[i] = 0.0;
            }
        }
    }
}

void training(const Config& config) {

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
    //{
    //    let mut file_buffer = create_bufwriter(&"models/bins.json".to_string());
    //    let json = serde_json::to_string(&bins).expect("Bins cannot be serialized.");
    //    file_buffer.write(json.as_ref()).unwrap();
    //}

    std::vector<Example> validation_set1;
    std::vector<Example> validation_set2;

    std::cout << "Starting the buffered loader" << std::endl;
    BufferLoader buffer_loader(
        config.buffer_size,
        config.batch_size,
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
        config.default_gamma
        );


    std::cout << "Start training" << std::endl;
    booster.training(validation_set1, validation_set2);
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

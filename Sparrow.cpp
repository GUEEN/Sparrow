// Sparrow.cpp : Defines the entry point for the console application.
//

#include "Matrix.h"
#include "Reader.h"

#include <string>

struct Config {
    /// File path to the training data
    std::string training_filename;
    /// Number of training examples
    int num_examples;
    /// Number of features
    int num_features;
    /// Range of the features for creating weak rules
    //  Range<int> range 
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
    "1",
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


int main() {
    Matrix X(32161, 123);
    //Matrix X(60000, 8);
    std::vector<int> y;

    std::string path = "C:\\DATA\\LIBSVM\\a9a";
    //std::string path = "C:\\DATA\\LIBSVM\\cod-rna";

    ReadTrainData(X, y, path);

    return 0;
}


#pragma once

#include <string>

#include "Bins.h"

struct Config {
    /// File path to the training data
    std::string training_filename;
    /// File path to the testing data
    std::string testing_filename;
    /// Number of training examples
    int num_examples;
    /// Number of features
    int num_testing_examples;
    /// Number of examples to scan for generating heuristic used in Sparrow
    int num_features;
    /// Range of the features for creating weak rules
    Range range;
    /// Label for positive examples
    std::string positive;
    /// Number of testing examples
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
};

Config read_config(const std::string& filename);


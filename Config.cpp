#include "Config.h"

#include <fstream>
#include <cassert>
#include <sstream>
#include <iterator>

std::string last_token(const std::string& line) {
    std::istringstream iss(line);
    std::vector<std::string> tokens(std::istream_iterator<std::string>{iss},
        std::istream_iterator<std::string>());
    return tokens.back();
}

bool stob(const std::string& word) {
    return word == "true";
}

//primitive yaml config reader. Order of elements matters
Config read_config(const std::string& filename) {
    std::ifstream f(filename);
    //f.open();

    std::string line;

    std::getline(f, line);
    assert(line == "---");
    std::getline(f, line);
    std::string training_filename = last_token(line);
    std::getline(f, line);
    std::string testing_filename = last_token(line);
    std::getline(f, line);
    int num_examples = stoi(last_token(line));
    std::getline(f, line);
    int num_testing_examples = stoi(last_token(line));
    std::getline(f, line);
    int num_features = stoi(last_token(line));
    std::getline(f, line);
    assert(line == "range:");
    std::getline(f, line);
    int range_begin = stoi(last_token(line));
    std::getline(f, line);
    int range_end = stoi(last_token(line));
    std::getline(f, line);
    std::string positive = last_token(line);
    std::getline(f, line);
    int max_sample_size = stoi(last_token(line));
    std::getline(f, line);
    int max_bin_size = stoi(last_token(line));
    std::getline(f, line);
    double min_gamma = stod(last_token(line));
    std::getline(f, line);
    double default_gamma = stod(last_token(line));
    std::getline(f, line);
    int max_trials_before_shrink = stoi(last_token(line));
    std::getline(f, line);
    double min_ess = stod(last_token(line));
    std::getline(f, line);
    int num_iterations = stoi(last_token(line));
    std::getline(f, line);
    int max_leaves = stoi(last_token(line));
    std::getline(f, line);
    int channel_size = stoi(last_token(line));
    std::getline(f, line);
    int buffer_size = stoi(last_token(line));
    std::getline(f, line);
    int batch_size = stoi(last_token(line));
    std::getline(f, line);
    bool serial_sampling = stob(last_token(line));
    std::getline(f, line);
    int num_examples_per_block = stoi(last_token(line));
    std::getline(f, line);
    std::string disk_buffer_filename = last_token(line);
    std::getline(f, line);
    int num_assigners = stoi(last_token(line));
    std::getline(f, line);
    int num_samplers = stoi(last_token(line));
    std::getline(f, line);
    bool save_process = stob(last_token(line));
    std::getline(f, line);
    int save_interval = stoi(last_token(line));
    std::getline(f, line);
    bool debug_mode = stob(last_token(line));

    return{
        training_filename,
        testing_filename,
        num_examples,
        num_testing_examples,
        num_features,
        {range_begin, range_end},
        positive, 
        max_sample_size,
        max_bin_size,
        min_gamma,
        default_gamma,
        max_trials_before_shrink,
        min_ess,
        num_iterations,
        max_leaves,
        channel_size,
        buffer_size,
        batch_size,
        serial_sampling,
        num_examples_per_block,
        disk_buffer_filename,
        num_assigners,
        num_samplers,
        save_process,
        save_interval,
        debug_mode
    };
}

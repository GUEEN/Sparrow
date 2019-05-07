#pragma once

#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <iterator>

#include "LabeledData.h"

template<class TFeature, class TLabel>
LabeledData<TFeature, TLabel> parse_libsvm_one_line(
    const std::string& raw_string,
    TFeature missing_val,
    int size,
    const std::string& positive) {

    std::istringstream iss(raw_string);
    std::vector<std::string> numbers(std::istream_iterator<std::string>{iss},
        std::istream_iterator<std::string>());

    TLabel label = numbers[0] == positive ? 1 : -1;

    std::vector<TFeature> feature(size, missing_val);
    std::vector<std::string> tokens;
    for (auto it = std::next(numbers.begin()); it != numbers.end(); ++it) {
        std::istringstream token_stream(*it);

        tokens.clear();
        std::string token;
        while (std::getline(token_stream, token, ':')) {
            tokens.push_back(token);
        }

        double value = std::stod(tokens[1]); // TODO: check the NAN value cases
        int index = std::stoi(tokens[0]);
        feature[index] = value;
    }
    return LabeledData<TFeature, TLabel>(feature, label);
}

template<class TFeature, class TLabel>
std::vector<LabeledData<TFeature, TLabel>> parse_libsvm(
    std::vector<std::string>& raw_strings,
    TFeature missing_val,
    int size,
    const std::string& positive) {
    std::vector<LabeledData<TFeature, TLabel>> data;

    for (const std::string& raw_string : raw_strings) {
        data.push_back(parse_libsvm_one_line<TFeature, TLabel>(raw_string, missing_val, size, positive));
    }
    return data;
}

class BufReader {
public:
    explicit BufReader(const std::string& filename);
    BufReader(const BufReader&) = default;
    BufReader(BufReader&&) = default;
    BufReader& operator=(const BufReader&) = default;
    BufReader& operator=(BufReader&&) = default;
    ~BufReader();

    Example read_exact();
    void read_line(std::string& line);

private:
    std::fstream f;
};

class BufWriter {
public:
    explicit BufWriter(const std::string& filename);
    BufWriter(const BufWriter&) = default;
    BufWriter(BufWriter&&) = default;
    BufWriter& operator=(const BufWriter&) = default;
    BufWriter& operator=(BufWriter&&) = default;
    ~BufWriter();

private:
    std::fstream f;
};


BufReader create_bufreader(const std::string& filename);
BufWriter create_bufwriter(const std::string& filename);

std::string read_all(const std::string& filename);
void write_all(const std::string& filename, const std::string& content);

std::vector<std::string> read_k_lines(BufReader& reader, int k);

template<class TFeature, class TLabel>
std::vector<LabeledData<TFeature, TLabel>> read_k_labeled_data(
    BufReader& reader,
    int k,
    TFeature missing_val,
    int size,
    const std::string& positive) {
    std::vector<std::string> lines = read_k_lines(reader, k);
    return parse_libsvm<TFeature, TLabel>(lines, missing_val, size, positive);
}

std::vector<Example> read_k_labeled_data_from_binary_file(
    BufReader& reader,
    int k,
    int data_size);

int write_to_binary_file(BufWriter& writer, const Example& data);

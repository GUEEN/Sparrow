#include "Io.h"

#include <iostream>
#include <stdlib.h>
#include <cassert>

BufReader::BufReader(const std::string& filename) {
    f.open(filename);
    if (f.good() == false) {
        std::cout << "FILE NOT FOUND!" << std::endl;
        exit(EXIT_FAILURE);
    }
}

BufReader::~BufReader() {
    f.close();
}

void BufReader::read_line(std::string& line) {
    std::getline(f, line);
}

// read an example from a binary file
Example BufReader::read_exact() {
    //TODO
    assert(false);
    int label = 0;
    std::vector<int> feature;
    
    return Example(feature, label);
}

BufWriter::BufWriter(const std::string& filename) {
    f.open(filename);
}

BufWriter::~BufWriter() {
    f.close();
}

BufReader create_bufreader(const std::string& filename) {
    return BufReader(filename);
}

BufWriter create_bufwriter(const std::string& filename) {
    return BufWriter(filename);
}

std::string read_all(const std::string& filename) {
    std::string contents;

    std::fstream file;
    file.open(filename);

    std::string line;
    while (std::getline(file, line)) {
        contents = contents + line + "\n";
    }
    file.close();
    return contents;
}

void write_all(const std::string& filename, const std::string& content) {
    std::fstream file;
    file.open(filename);
    file << content;
    file.close();
}

std::vector<std::string> read_k_lines(BufReader& reader, int k) {
    std::vector<std::string> ret;
    ret.reserve(k);

    for (int i = 0; i < k; ++i) {
        std::string line;
        reader.read_line(line);
        if (line != "") {
            ret.push_back(line);
        } else {
            return ret;
        }
    }
    return ret;
}

std::vector<Example> read_k_labeled_data_from_binary_file(
    BufReader& reader, int k, int data_size) {

    std::vector<Example> data;

    for (int i = 0; i < k; ++i) {
        data.push_back(reader.read_exact());
    }

    return data;
}

int write_to_binary_file(BufWriter& writer, const Example& data) {
    int total_bytes = 0;
    // TODO
    return total_bytes;
}


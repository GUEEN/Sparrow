#include "Io.h"

BufReader::BufReader(const std::string& filename) {
    f.open(filename);
}

BufReader::~BufReader() {
    f.close();
}

std::string BufReader::read_line() {
    std::string line;
    std::getline(f, line);
    return line;
}

// read an example from a binary file
Example BufReader::read_exact() {
    //TODO
    int label = 0;
    std::vector<int> feature;
    
    return Example(feature, label);
}

std::vector<Example> read_k_labeled_data_from_binary_file(
    BufReader& reader, int k, int data_size) {
    std::vector<Example> data;
    // TODO
    return data;
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
    std::vector<std::string> ret(k);

    for (int i = 0; i < k; ++i) {
        ret[i] = reader.read_line();
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


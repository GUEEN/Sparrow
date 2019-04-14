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



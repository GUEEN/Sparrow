#include "Reader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>

// rows and columns must be already set
void ReadTrainData(Matrix& data, std::vector<int>& labels, const std::string& path) {

    std::vector<std::string> tokens(2);
    std::fstream file;
    file.open(path);

    std::string line;
    int row_index = 0;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::vector<std::string> line_split(std::istream_iterator<std::string>{iss}, 
            std::istream_iterator<std::string>());
        std::string label = line_split[0];
        if (label[0] == '+') {
            labels.push_back(1);
        } else {
            labels.push_back(-1);
        }

        std::string token;
        std::vector<std::string> tokens;
        for (auto it = std::next(line_split.begin()); it != line_split.end(); ++it) {
            std::istringstream token_stream(*it);

            tokens.clear();
            while (std::getline(token_stream, token, ':')) {
                tokens.push_back(token);
            }

            double value = std::stod(tokens[1]);
            int col_index = std::stoi(tokens[0]) - 1;
            data.add(value, row_index, col_index);
        }
        ++row_index;
    }
    file.close();
}





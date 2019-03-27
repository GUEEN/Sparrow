#pragma once

#include <string>
#include "Matrix.h"

// read the data from a libsvm file and fill the data matrix
void ReadTrainData(Matrix& data, std::vector<int>& labels, const std::string& path);
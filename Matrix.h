#pragma once

#include <vector>
#include <unordered_map>

// a sparse matrix data class

class Matrix {
public:
    Matrix(int rows, int cols);
    void add(double value, int row_index, int col_index);
    double operator()(int x, int y) const;

private:

    int rows;
    int cols;

    std::vector<double> values;
    std::vector<int> row_indices;
    std::vector<int> col_indices;

    std::unordered_map<int, int> map;

    const int nullval = -1;
};

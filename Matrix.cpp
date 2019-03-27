#include "Matrix.h"

Matrix::Matrix(int rows, int cols) : rows(rows), cols(cols) {}

void Matrix::add(double value, int row_index, int col_index) {
    map[rows * col_index + row_index] = values.size();

    row_indices.push_back(row_index);
    col_indices.push_back(col_index);
    values.push_back(value);
}

double Matrix::operator()(int x, int y) const {
    int index = rows * y + x;
    auto p = map.find(index);
    if (p == map.end()) {
        return nullval;
    } else {
        return p->second;
    }
}
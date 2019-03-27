// Sparrow.cpp : Defines the entry point for the console application.
//

#include "Matrix.h"
#include "Reader.h"

int main() {
    Matrix X(32161, 123);
    std::vector<int> y;

    std::string path = "C:\\DATA\\LIBSVM\\a9a";

    ReadTrainData(X, y, path);

    return 0;
}


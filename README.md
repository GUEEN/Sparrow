# Sparrow

A C++ implementation of the Sparrow boosting algorithm following
https://arxiv.org/abs/1901.09047

## Usage

To build sparrow simply run `make`. To run provide the path to the config file

```bash
sparrow <path to config>
```
This implementation does not use any external libraries. All the training and testing data must be in [LIBSVM](https://www.csie.ntu.edu.tw/~cjlin/libsvmtools/datasets/binary.html) format

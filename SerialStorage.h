#include <string>

class SerialStorage {

private:
    std::string filename;
    bool is_binary;
    bool in_memory;
    int size;
    int feature_size;
    std::string positive;

    int bytes_per_example;

    int head;
    int tail;
};
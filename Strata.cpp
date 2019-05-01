#include "Strata.h"

#include <cassert>
#include <mutex>

using std::lock_guard;

void Strata::send(int index, const Example& example, double score, int version) {
    assert(index >= -128 && index <= 127);
    index += 128;

    lock_guard<std::mutex> lock(this->mutex);
    auto& sender = in_queues[index];
    if (sender.get() == nullptr) {
        this->create(index);
    }
    sender->send({ example,{ score, version } });
}

void Strata::create(int index) {
    // TODO!!!
    assert(false);
}

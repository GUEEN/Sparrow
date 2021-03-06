#pragma once

// TODO!!!!!!!!!!!!!!!!!!!!!!!!
// make sure this works
// open questions:
//   - which send/recv is blocking/non-blocking
//   - when is the channel closed?

#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <array>

#include "LabeledData.h"
#include "ThreadManager.h"

enum Signal { START, STOP };

template<typename T>
class Channel {
public:

    Channel(const std::string& name, int size) : name(name), size(size) {
        std::cout << "Channel " << name << " is created" << std::endl;
    }

    ~Channel() {
        std::cout << "Channel " << name << " is being deleted" << std::endl;
    }

    void send(const T& value) {
        while (ThreadManager::continue_run) {
            {
                std::lock_guard<std::mutex> lock(mutex);
                if (size == 0 || q.size() < size) {
                    q.push(value);
                    return;
                }
            }
            std::this_thread::yield();
        }
    }

    T recv() {
        while (ThreadManager::continue_run) {
            mutex.lock();
            if (q.empty()) {
                mutex.unlock();
                std::this_thread::yield();
                continue;
            }

            T value = q.front();
            q.pop();
            mutex.unlock();

            return value;
        }
        return T();
    }

    std::pair<bool, T> try_recv() {
        std::lock_guard<std::mutex> lock(mutex);
        if (q.empty()) {
            return std::make_pair(false, T());
        }
        T value = q.front();
        q.pop();
        return std::pair<bool, T>(true, value);
    }

    std::string get_name() const {
        return name;
    }

    int len() const {
        std::lock_guard<std::mutex> lock(mutex);
        return q.size();
    }

private:
    std::string name;
    mutable std::mutex mutex;
    std::queue<T> q;
    const int size;

    Channel() = delete;
    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;
};

template<typename T>
struct Sender {
public:
    explicit Sender(std::shared_ptr<Channel<T>> chan) : chan(chan) {}
    void send(const T& value) {
        chan->send(value);
    }
    ~Sender() {
        //std::cout << "Deleting last sender for channel '" << chan->get_name() << "'" << std::endl;
    }
private:
    Sender() = delete;
    std::shared_ptr<Channel<T>> chan;
};

template<typename T>
struct Receiver {
public:
    explicit Receiver(std::shared_ptr<Channel<T>> chan) : chan(chan) {}
    T recv() {
        return chan->recv();
    }
    std::pair<bool, T> try_recv() {
        return chan->try_recv();
    }
    ~Receiver() {
        //std::cout << "Deleting last receiver for channel '" << chan->get_name() << "'" << std::endl;
    }

    int len() const {
        return chan->len();
    }

private:
    Receiver() = delete;
    std::shared_ptr<Channel<T>> chan;
};

template<typename T>
std::pair<Sender<T>, Receiver<T>> unbounded_channel(const std::string& name) {
    std::shared_ptr<Channel<T>> chan(new Channel<T>(name, 0));
    return std::make_pair(Sender<T>(chan), Receiver<T>(chan));
}

template<typename T>
std::pair<Sender<T>, Receiver<T>> bounded_channel(int size, const std::string& name) {
    // TODO: make channel actually bounded
    std::shared_ptr<Channel<T>> chan(new Channel<T>(name, size));
    return std::make_pair(Sender<T>(chan), Receiver<T>(chan));
}

typedef Sender<ExampleWithScore> InQueueSender;
typedef Receiver<ExampleWithScore> OutQueueReceiver;

typedef std::array<std::unique_ptr<InQueueSender>, 256> HashMapSenders;
typedef std::array<std::unique_ptr<OutQueueReceiver>, 256> HashMapReceiver;

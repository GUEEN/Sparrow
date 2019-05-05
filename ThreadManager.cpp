#include "ThreadManager.h"

#include <iostream>

std::map<std::thread::id, bool> ThreadManager::run;
std::map<std::thread::id, bool> ThreadManager::stopped;
bool ThreadManager::finish = false;

void ThreadManager::add(std::thread::id id) {
    stopped[id] = false;
    if (finish == false) {
        run[id] = true;
    } else {
        run[id] = false;
    }
}

bool ThreadManager::continue_run(std::thread::id id) {
    if (run.find(id) == run.end())
        return true;
    return run[id];
}

void ThreadManager::done(std::thread::id id) {
    stopped[id] = true;
}

void ThreadManager::stop(std::thread::id id) {
    run[id] = false;
    while (!stopped[id]);
}

void ThreadManager::stop_all() {
    finish = true;
    for (auto& p : run) {
        p.second = false;
    }

    bool all_stopped = false;
    while (!all_stopped) {
        all_stopped = true;
        for (auto& p : stopped) {
            all_stopped = all_stopped && p.second;
        }
    }
    std::cout << "All extra threads have been stopped" << std::endl;
}
#pragma once

#include <map>
#include <thread>

class ThreadManager {
public:
    static void add(std::thread::id id);
    static bool continue_run(std::thread::id id);
    static void done(std::thread::id id);
    static void stop(std::thread::id id);
    static void stop_all();

private:
    static std::map<std::thread::id, bool> run;
    static std::map<std::thread::id, bool> stopped;
    static bool finish;
};

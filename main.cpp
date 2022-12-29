#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <stdint.h>
#include <signal.h>
#include "kafkalog.h"

bool g_running = true;
extern GlobalConfig g_config;

void handle_signal(int sig) {
    std::cout << "receive sig " << sig << "\n";
    g_running = false;
}

void init_log() {
    spdlog::set_pattern("%v");
    spdlog::flush_every(std::chrono::seconds(3));
}

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, handle_signal);

    if (!parse_config()) {
        std::cout << "config parse error\n";
        return -1;
    }

    init_log();

    // start topic pthreads
    std::vector<std::thread> writelog_threads;
    for (auto & topicinfo : g_config._topicinfo) {
        for (auto & topic : topicinfo._topics) {
            writelog_threads.emplace_back(std::thread(kafka_consume, topicinfo, topic));
        }
    }

    while (g_running) {
        update_time();
        sleep(1);
    }

    // join all threads
    for (auto & thread : writelog_threads) {
        thread.join();
    }

    std::cout << "kafka-log exit\n";
    return 0;
}


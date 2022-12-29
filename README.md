# About the *kafka-log*



## Introduction

The kafka-log is is a high-performance application thats receive kafka message and write logs to disk. 

The app parse  app.yaml, according to `topic-services` generate kafka topics, you can modify `config.h` to generate other rule kafka topics. 

The app create a thread for each group kafka of topic to receive kafka message, app generates an absolute PATH according to some header fields of the kafka topic, you can modify function `write_log` in the file  `kafkalog.h` file to generate other PATH, Finally app write log to disk.



## Requires

- C++17
- [modern-cpp-kafka](https://github.com/morganstanley/modern-cpp-kafka)
- [spdlog](https://github.com/gabime/spdlog)
- [yaml-cpp](https://github.com/jbeder/yaml-cpp)



## Build

You can use the CentOS8 Docker image, install gcc/g++and Requirements lib, and then make.


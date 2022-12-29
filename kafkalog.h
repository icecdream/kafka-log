#pragma once

#include <string>
#include <vector>
#include <thread>
#include <unordered_map>

#include "config.h"
#include "common.h"
#include "kafka/KafkaConsumer.h"
#include "spdlog/spdlog.h"
#include "spdlog/logger.h"
#include "spdlog/sinks/basic_file_sink.h" // support for basic file logging

using namespace kafka;
using namespace kafka::clients;
using namespace kafka::clients::consumer;


extern GlobalConfig g_config;
extern bool g_running;

// key: filename, key2: date
thread_local std::unordered_map<std::string, std::pair<std::string, std::shared_ptr<spdlog::logger>>> g_all_logger;


void write_log(const TopicInfo& topicInfo, const consumer::ConsumerRecord& record);

void kafka_consume(const TopicInfo& topicInfo, const std::set<std::string> &topics)
{
    std::string strtopic;
    for (auto & topic : topics) {
        strtopic += topic + ",";
    }
    pthread_setname_np(pthread_self(), strtopic.c_str()); 

    std::string last_check_hour = get_hour();

    while (g_running) {
        try {
            // Create configuration object
            Properties props ({
                {"bootstrap.servers", {g_config._brokers}}
            });
            // props.put(ConsumerConfig::AUTO_OFFSET_RESET, "earliest");
            props.put(ConsumerConfig::GROUP_ID, g_config._groupid);

            // Create a consumer instance
            KafkaConsumer consumer(props);

            // Subscribe to topics
            consumer.subscribe(topics);
            // consumer.seekToEnd();

            // Read messages from the topic
            std::cout << "Reading groupid: " << g_config._groupid << ", messages from topic: " << strtopic << std::endl;

            while (g_running) {
                const std::string &cur_hour = get_hour();
                if (last_check_hour != cur_hour) {
                    // 清理过期文件, 避免占用磁盘
                    for (auto itr=g_all_logger.begin(); itr!=g_all_logger.end(); ) {
                        if (itr->second.first != cur_hour) {
                            std::cout << "remove file: " << itr->first << std::endl;
                            spdlog::drop(itr->first);
                            g_all_logger.erase(itr++);
                        } else {
                            ++itr;
                        }
                    }
                    last_check_hour = cur_hour;
                }

                const auto& records = consumer.poll(std::chrono::milliseconds(100));
                for (const auto& record: records) {
                    // In this example, quit on empty message
                    if (record.value().size() == 0) {
                        // std::cout << "WARN record value size 0 >> Topic: " << record.topic() << ", Key: " << record.key().toString() << ", Headers: " << toString(record.headers()) << std::endl;
                        continue;
                    }

                    if (!record.error()) {
                        write_log(topicInfo, record);

                    } else {
                        std::cout << "ERROR record >> Topic: " << record.topic() << ", Key: " << record.key().toString() << ", Headers: " << toString(record.headers()) << std::endl;
                    }
                }
            }

            // consumer.close(); // No explicit close is needed, RAII will take care of it

        } catch (const KafkaException& e) {
            // 避免小概率超时, 继续消费数据
            std::cout << "ERROR exception caught: " << e.what() << std::endl;
            sleep(5);
        }
    }
}

static std::string KAFKA_HEADER_APP("app");
static std::string KAFKA_HEADER_POD("pod");
static std::string KAFKA_HEADER_TP("tp");

void write_log(const TopicInfo& topicInfo, const consumer::ConsumerRecord& record)
{
    // eg headers: pod:server-a-69f85464f4-dc5z4,node:10.104.2.205,tp:need.log,app:server-a
    std::string_view record_app, record_pod, record_tp;
    for (auto & header : record.headers()) {
        if (KAFKA_HEADER_APP == header.key) {
            record_app = std::string_view((const char*)(header.value.data()), header.value.size());

        } else if (KAFKA_HEADER_POD == header.key) {
            record_pod = std::string_view((const char*)(header.value.data()), header.value.size());

        } else if (KAFKA_HEADER_TP == header.key) {
            record_tp = std::string_view((const char*)(header.value.data()), header.value.size());
            if (topicInfo._filter_log.count(std::string(record_tp))) {
                // std::cout << "record tp: " << record_tp << " need filter\n";
                return ;
            }
        }
    }

    /*
    std::cout << "Got a new message..." << std::endl;
    std::cout << "    Topic    : " << record.topic() << std::endl;
    std::cout << "    Partition: " << record.partition() << std::endl;
    std::cout << "    Offset   : " << record.offset() << std::endl;
    std::cout << "    Timestamp: " << record.timestamp().toString() << std::endl;
    std::cout << "    Headers  : " << toString(record.headers()) << std::endl;
    std::cout << "    Key   [" << record.key().toString() << "]" << std::endl;
    std::cout << "    Value [" << record.value().toString() << "]" << std::endl;
    */

    // /home/server/logs/server-a/20221224/2022122417/need.log.server-a-69f85464f4-dc5z4
    auto strsize = g_config._logpath_prefix.length() + record_app.length() + record_tp.length() + record_pod.length() + 23;
    std::string file_name;
    file_name.reserve(strsize); 
    file_name.append(g_config._logpath_prefix).append(record_app).append("/").append(get_date()).append("/").append(get_hour()).append("/").append(record_tp).append(".").append(record_pod);

    auto& logger = g_all_logger[file_name];
    if (logger.second.get() == NULL) {
        std::cout << "create " << logger.first << " file: " << file_name << "\n";

        logger.second = spdlog::get(file_name);
        if(logger.second.get() == NULL) {
            try {
                // _mt线程安全, _st非线程安全
                logger.second = spdlog::basic_logger_st(file_name, file_name);

            } catch (const std::exception& e) {
                // 避免路径中一些字段有特殊字段, 导致file_name是目录, 创建文件抛异常
                std::cout << "ERROR basic_logger_st filename: " << file_name << ", except caught: " << e.what() << std::endl;
                return ;
            }
        }
        logger.first = get_hour();
    }

    // std::cout << record.value().toString() << "\n";
    std::string_view logdata((const char*)(record.value().data()), record.value().size());
    logger.second->info(logdata);
}


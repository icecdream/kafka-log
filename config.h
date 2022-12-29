#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include "yaml-cpp/yaml.h"

const std::string CONF_LOGPATH_PREFIX = "log-path-prefix";
const std::string CONF_GROUP_ID = "groupId";
const std::string CONF_TOPIC_SERVICES = "topic-services";
const std::string CONF_BROKERS = "brokers";
const std::string CONF_FILTER_LOG = "filter-log";

struct TopicInfo {
    std::string _service;
    std::vector<std::set<std::string>> _topics;
    // dont write log
    std::unordered_set<std::string> _filter_log;
};

struct GlobalConfig {
    std::string _logpath_prefix;

    // kafka
    std::string _groupid;
    std::string _brokers;
    std::vector<TopicInfo> _topicinfo;


    void debug_string() {
        std::cout << "====== config ======\n" << "logpath prefix: " << _logpath_prefix << "groupid: " << _groupid << "\n" << "brokers: " << _brokers << "\n";
        for (auto & topicinfo : _topicinfo) {
            std::cout << "topicinfo: " << topicinfo._service << "\n";
            for (auto & topics : topicinfo._topics) {
                for (auto & topic : topics) {
                    std::cout << topic << ", ";
                }
                std::cout << std::endl;
            }

            if (!topicinfo._filter_log.empty()) {
                std::cout << "filter log: ";
                for (auto & filter : topicinfo._filter_log) {
                    std::cout << filter << ", ";
                }
                std::cout << std::endl;

            } else {
                std::cout << "filter log empty\n";
            }
            std::cout << std::endl;
        }
        std::cout << "====== config end ======\n";
    }
};

GlobalConfig g_config;

bool parse_config()
{
    YAML::Node config = YAML::LoadFile("conf/app.yaml");
    if (!config["kafka"]) {
        std::cout << "conf need kafka\n";
        return false;
    }

    YAML::Node conf_kafka = config["kafka"];
    g_config._logpath_prefix = conf_kafka[CONF_LOGPATH_PREFIX].as<std::string>();
    g_config._groupid = conf_kafka[CONF_GROUP_ID].as<std::string>();

    // parse brokers
    if (!conf_kafka[CONF_BROKERS] || !conf_kafka[CONF_BROKERS].IsSequence()) {
        std::cout << "conf " << CONF_BROKERS << " type error\n";
        return false;
    }
    for (YAML::const_iterator it = conf_kafka[CONF_BROKERS].begin(); it != conf_kafka[CONF_BROKERS].end(); ++it) {
        g_config._brokers += it->as<std::string>() + ",";
    }
    if (!g_config._brokers.empty()) {
        g_config._brokers.pop_back();    // remove end ,
    }

    // parse topic services
    if (!conf_kafka[CONF_TOPIC_SERVICES] || !conf_kafka[CONF_TOPIC_SERVICES].IsMap()) {
        std::cout << "conf " << CONF_TOPIC_SERVICES << " type error\n";
        return false;
    }
    for (YAML::const_iterator it = conf_kafka[CONF_TOPIC_SERVICES].begin(); it != conf_kafka[CONF_TOPIC_SERVICES].end(); ++it) {
        std::string service = it->first.as<std::string>();

        // topics
        TopicInfo topicInfo;
        topicInfo._service = service;
        topicInfo._topics.push_back({"kube-topic-" + service});
        topicInfo._topics.push_back({"kube-topic-" + service + "-notice"});

        // filter
        YAML::Node conf_service = it->second;
        if (conf_service[CONF_FILTER_LOG] && conf_service[CONF_FILTER_LOG].IsSequence()) {
            for (YAML::const_iterator it_filter = conf_service[CONF_FILTER_LOG].begin(); it_filter != conf_service[CONF_FILTER_LOG].end(); ++it_filter) {
                topicInfo._filter_log.insert(it_filter->as<std::string>());
            }
        }

        g_config._topicinfo.emplace_back(topicInfo);
    }

    g_config.debug_string();
    return true;
}


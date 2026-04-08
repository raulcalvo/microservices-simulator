#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <nlohmann/json.hpp>
#include "messages.hpp"

class Logger {
public:
    static void init() {
        // We only want pure JSON output for structured logging
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern("%v");
        spdlog::logger logger("json_logger", {console_sink});
        spdlog::set_default_logger(std::make_shared<spdlog::logger>(logger));
        spdlog::set_level(spdlog::level::info);
    }

    static void log_event(const Message& msg) {
        nlohmann::json j = msg;
        spdlog::info(j.dump());
    }

    static void log_custom(const std::string& timestamp, const std::string& correlation_id, const std::string& flow_id, const std::string& service, const std::string& event, const nlohmann::json& payload) {
        Message msg{timestamp, correlation_id, flow_id, service, event, payload};
        log_event(msg);
    }
};

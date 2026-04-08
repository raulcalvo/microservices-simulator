#pragma once
#include <string>
#include <nlohmann/json.hpp>

// Core message structure passing between nodes
struct Message {
    std::string timestamp;
    std::string correlation_id;
    std::string flow_id;
    std::string service;
    std::string event;
    nlohmann::json payload;

    // Macro to automatically serialize/deserialize JSON for this struct
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Message, timestamp, correlation_id, flow_id, service, event, payload)
};

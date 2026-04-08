#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <random>
#include <cxxopts.hpp>
#include <zmq.hpp>
#include "core/logger.hpp"
#include "core/messages.hpp"
#include "nlohmann/json.hpp"

// Utility: get current timestamp
std::string get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return std::to_string(ms); // simplified timestamp
}

// Utility: Generate basic Correlation UUID
std::string generate_uuid() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    const char* v = "0123456789abcdef";
    std::string res;
    for (int i = 0; i < 32; i++) {
        res += v[dis(gen)];
        if (i == 8 || i == 12 || i == 16 || i == 20) res += "-";
    }
    return res;
}

// Utility: Calculate chaos
bool should_fail(int percentage) {
    if (percentage <= 0) return false;
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);
    return dis(gen) <= percentage;
}

void send_message(zmq::socket_t& socket, const Message& msg) {
    nlohmann::json j = msg;
    std::string payload_str = j.dump();
    zmq::message_t zmq_msg(payload_str.size());
    memcpy(zmq_msg.data(), payload_str.c_str(), payload_str.size());
    socket.send(zmq_msg, zmq::send_flags::none);
    Logger::log_event(msg);
}

bool recv_message(zmq::socket_t& socket, Message& msg, bool non_blocking = false) {
    zmq::message_t zmq_msg;
    auto res = socket.recv(zmq_msg, non_blocking ? zmq::recv_flags::dontwait : zmq::recv_flags::none);
    if (!res || res.value() == 0) return false;
    
    std::string payload_str(static_cast<char*>(zmq_msg.data()), zmq_msg.size());
    nlohmann::json j = nlohmann::json::parse(payload_str);
    msg = j.get<Message>();
    msg.timestamp = get_current_timestamp();
    msg.event = "MESSAGE_RECEIVED";
    Logger::log_event(msg);
    return true;
}

// Node behaviors mapped dynamically
int main(int argc, char** argv) {
    cxxopts::Options options("micro_node", "A dummy distributed microservices system simulator");
    options.add_options()
        ("role", "Node role (producer, aggregator, router...)", cxxopts::value<std::string>())
        ("flow", "Flow ID string", cxxopts::value<std::string>())
        ("chaos", "Enable chaos", cxxopts::value<bool>()->default_value("false"))
        ("bind", "Bind address (optional)", cxxopts::value<std::string>()->default_value(""))
        ("connect1", "Connect address 1 (optional)", cxxopts::value<std::string>()->default_value(""))
        ("connect2", "Connect address 2 (optional)", cxxopts::value<std::string>()->default_value(""));

    auto result = options.parse(argc, argv);
    std::string role = result["role"].as<std::string>();
    std::string flow = result["flow"].as<std::string>();
    bool chaos = result["chaos"].as<bool>();
    std::string bind_addr = result["bind"].as<std::string>();
    std::string conn1 = result["connect1"].as<std::string>();
    std::string conn2 = result["connect2"].as<std::string>();

    Logger::init();
    zmq::context_t ctx;

    // A generic setup depending on if we are bind/connect
    // REQ/REP or PUSH/PULL. We'll use PUSH/PULL as base to keep this PoC simple,
    // and dynamically change for Scatter-gather.

    // Let's implement specific roles:
    if (role == "producer") {
        zmq::socket_t push_sock(ctx, zmq::socket_type::push);
        if (!conn1.empty()) push_sock.connect(conn1);
        
        while (true) {
            Message msg;
            msg.timestamp = get_current_timestamp();
            msg.correlation_id = generate_uuid();
            msg.flow_id = flow;
            msg.service = role;
            msg.event = "MESSAGE_SENT";
            msg.payload = {{"tenant_id", "A123"}, {"value", rand() % 100}};
            
            if (flow.find("ROUTER") != std::string::npos) {
                msg.payload["tier"] = (rand() % 2 == 0) ? "STANDARD" : "VIP";
            }
            
            send_message(push_sock, msg);
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    } 
    else if (role == "aggregator") {
        zmq::socket_t pull_sock(ctx, zmq::socket_type::pull);
        if (!bind_addr.empty()) pull_sock.bind(bind_addr);
        
        std::vector<Message> batch;
        while (true) {
            Message msg;
            if (recv_message(pull_sock, msg)) {
                batch.push_back(msg);
                if (batch.size() >= 10) {
                    Logger::log_custom(get_current_timestamp(), msg.correlation_id, flow, role, "BATCH_PROCESSED", {{"batch_size", 10}});
                    batch.clear();
                }
            }
        }
    }
    else if (role == "slowprocessor") {
        zmq::socket_t pull_sock(ctx, zmq::socket_type::pull);
        pull_sock.bind(bind_addr);
        while (true) {
            Message msg;
            if (recv_message(pull_sock, msg)) {
                if (chaos && should_fail(5)) {
                    Logger::log_custom(get_current_timestamp(), msg.correlation_id, flow, role, "CHAOS_SLEEP_2S", {});
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                }
                msg.event = "PROCESSED_SLOWLY";
                Logger::log_event(msg);
            }
        }
    }
    else if (role == "worker") {
        zmq::socket_t pull_sock(ctx, zmq::socket_type::pull);
        pull_sock.bind(bind_addr);
        while (true) {
            Message msg;
            if (recv_message(pull_sock, msg)) {
                Logger::log_custom(get_current_timestamp(), msg.correlation_id, flow, role, "JOB_STARTED", {});
                if (chaos && should_fail(10)) {
                    continue; // Early return without JOB_FINISHED
                }
                Logger::log_custom(get_current_timestamp(), msg.correlation_id, flow, role, "JOB_FINISHED", {});
            }
        }
    }
    else if (role == "router") {
        zmq::socket_t pull_sock(ctx, zmq::socket_type::pull);
        pull_sock.bind(bind_addr);
        
        zmq::socket_t std_sock(ctx, zmq::socket_type::push);
        std_sock.connect(conn1);
        zmq::socket_t vip_sock(ctx, zmq::socket_type::push);
        std_sock.connect(conn2);

        while (true) {
            Message msg;
            if (recv_message(pull_sock, msg)) {
                std::string tier = msg.payload.value("tier", "STANDARD");
                if (chaos && should_fail(10)) { // randomly routes VIP to STD
                    tier = "STANDARD";
                }
                
                msg.service = role;
                msg.event = "ROUTED_" + tier;
                if (tier == "STANDARD") send_message(std_sock, msg);
                else send_message(vip_sock, msg);
            }
        }
    }
    else if (role == "orchestrator") {
        while (true) {
            zmq::socket_t bill_sock(ctx, zmq::socket_type::req);
            bill_sock.connect(conn1);
            zmq::socket_t inv_sock(ctx, zmq::socket_type::req);
            inv_sock.connect(conn2);

            std::this_thread::sleep_for(std::chrono::seconds(3));
            Message start_msg{get_current_timestamp(), generate_uuid(), flow, role, "SCATTER_REQUEST", {{"user_id","123"}}};
            Logger::log_event(start_msg);
            
            // Scatter
            send_message(bill_sock, start_msg);
            if (!chaos) { // In chaos, orchestrator proceeds without waiting for billing
                Message bill_resp;
                recv_message(bill_sock, bill_resp);
            }
            send_message(inv_sock, start_msg);
            Message inv_resp;
            recv_message(inv_sock, inv_resp);
            
            Logger::log_custom(get_current_timestamp(), start_msg.correlation_id, flow, role, "GATHER_COMPLETE", {});
        }
    }
    else if (role == "billing" || role == "inventory") {
        zmq::socket_t rep_sock(ctx, zmq::socket_type::rep);
        rep_sock.bind(bind_addr);
        while (true) {
            Message msg;
            if (recv_message(rep_sock, msg)) {
                msg.event = role + "_VALIDATED";
                msg.service = role;
                send_message(rep_sock, msg);
            }
        }
    }
    else if (role == "node2") { // Corruption scenario
        zmq::socket_t pull_sock(ctx, zmq::socket_type::pull);
        pull_sock.bind(bind_addr);
        zmq::socket_t push_sock(ctx, zmq::socket_type::push);
        push_sock.connect(conn1);

        while (true) {
            Message msg;
            if (recv_message(pull_sock, msg)) {
                if (chaos) {
                    msg.payload.erase("tenant_id"); // delete tenant_id
                }
                msg.service = role;
                msg.event = "FORWARDED";
                send_message(push_sock, msg);
            }
        }
    }
    else {
        // Generic forwarder like enricher, consumer, node1, node3
        zmq::socket_t pull_sock(ctx, zmq::socket_type::pull);
        if (!bind_addr.empty()) pull_sock.bind(bind_addr);
        
        std::unique_ptr<zmq::socket_t> push_sock;
        if (!conn1.empty()) {
            push_sock = std::make_unique<zmq::socket_t>(ctx, zmq::socket_type::push);
            push_sock->connect(conn1);
        }

        while (true) {
            Message msg;
            if (recv_message(pull_sock, msg)) {
                if (push_sock) {
                    msg.service = role;
                    msg.event = "FORWARDED";
                    send_message(*push_sock, msg);
                }
            }
        }
    }

    return 0;
}

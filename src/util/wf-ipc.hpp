#ifndef WF_IPC_HPP
#define WF_IPC_HPP

#include <condition_variable>
#include <functional>
#include <mutex>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <thread>
#include <nlohmann/json.hpp>
#include <vector>

class WayfireIPC
{
        int socket_fd;
        std::thread receiver;
        std::condition_variable resp_flag;
        std::mutex resp_mutex;
        std::string last_response;
        bool is_waiting = false;
        std::vector<std::function<void(nlohmann::json)>> handlers;

        void connect();
        void disconnect();
        std::string receive_message(int timeout_ms);
        void receive_loop();
        void return_response(std::string message);
    public:
        void send();
        void receive();
        void send_message(const std::string& message, int timeout_ms);
        std::string wait_response();
        void add_handler(std::function<void(nlohmann::json)>);
        WayfireIPC();
        ~WayfireIPC();
};

#endif // WF_IPC_HPP
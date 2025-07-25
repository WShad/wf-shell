#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <mutex>
#include <nlohmann/json_fwd.hpp>
#include <ostream>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>
#include <vector>
#include <wayfire/nonstd/json.hpp>

#include "wf-ipc.hpp"

WayfireIPC::WayfireIPC(): socket_fd(-1)
{
    connect();

    receiver = std::thread(&WayfireIPC::receive_loop, this);
    
    // send();
    // std::string message = "{\"method\":\"wayfire/get-keyboard-state\"}";
    std::string message = "{\"method\":\"window-rules/events/watch\"}";

    send_message(message, 2000);
    message = wait_response();
    // receive();
    // message = receive_message(2000);
    std::cout<<message<<std::endl;
    // wf::json_t test;
    // auto err = wf::json_t::parse_string(message, test);
    // if (err.has_value()) {
    //     std::cout<<"Error parsing JSON: "<<*err<<std::endl; 
    // }

    nlohmann::json test(message);

}

WayfireIPC::~WayfireIPC()
{
    disconnect();
}

void WayfireIPC::connect()
{
    const char* socket_path = getenv("WAYFIRE_SOCKET");
    if (socket_path == nullptr)
    {

    }

    socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {

    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

    if (strlen(socket_path) >= sizeof(addr.sun_path))
    {

    }

    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    if (::connect(socket_fd, (sockaddr*)&addr, sizeof(addr)) == -1)
    {

    }
}

void WayfireIPC::disconnect()
{
    if (socket_fd != -1)
    {
        close(socket_fd);
        socket_fd = -1;
    }
}

void WayfireIPC::send()
{
    std::string message = "{\"method\":\"wayfire/get-keyboard-state\"}";
    auto len = message.size();
    ::send(socket_fd, (char*)&len, 4, 0);
    auto res = ::send(socket_fd, message.c_str(),  message.size(), 0);
}

void WayfireIPC::receive()
{
    size_t len = 0;
    auto i = recv(socket_fd, &len, 4, 0);
    if (i <= 0)
    {
        std::cout<<"Receiving size: "<<i<<std::endl;
        return;
    }

    std::cout<<"Header got: "<<len<<std::endl;

    std::vector<char> buf(len + 1, 0); 

    std::cout<<"Buffer ready"<<std::endl;

    // buf.resize(len);
    i = recv(socket_fd, &buf[0], len, 0);
    std::cout<<"Recv finished: "<<i<<std::endl;
    if (i <= 0)
    {
        std::cout<<"Receiving data: "<<i<<std::endl;
        return;
    }

    // sleep(2);
    auto str = std::string_view(buf.data());
    std::cout<<"Converted"<<std::endl;

    std::cout<<"Received answer: "<<str<<std::endl;
}

void WayfireIPC::return_response(std::string message) {
    std::lock_guard<std::mutex> lock(resp_mutex);
    if (is_waiting) {
        last_response = message;
        resp_flag.notify_one();
    }
}

std::string WayfireIPC::wait_response() {
    std::unique_lock<std::mutex> lock(resp_mutex);
    // if time_out > 0
    is_waiting = true;
    resp_flag.wait(lock);
    is_waiting = false;
    return last_response;
}

void WayfireIPC::add_handler(std::function<void(nlohmann::json data)> handler) {
    handlers.push_back(handler);
}

void WayfireIPC::receive_loop() {
    while (true) {
        auto message = receive_message(-1);
        std::cout<<"Message received: "<<message<<std::endl;
        auto message_data = nlohmann::json::parse(message);
        if (message_data.contains("event")) {
            std::cout<<"Received event: "<<message_data["event"]<<std::endl;

            for (auto handler : handlers) {
                handler(message_data);
                continue;
            }
        }

        return_response(message);
    }
}
    //  send_json(const json& data, int timeout_ms = 2000)
    // {
    //     std::lock_guard<std::mutex> lock(sock_mutex);
        
    //     // Сериализация с обработкой ошибок
    //     std::string message;
    //     try {
    //         message = data.dump();
    //     } catch (const json::exception& e) {
    //         throw std::runtime_error("JSON serialization failed: " + std::string(e.what()));
    //     }
    //     message += "\n";  // Добавляем разделитель сообщений

    //     // Отправка данных
    //     send_message(message, timeout_ms);

    //     // Для команд ожидаем ответ
    //     if (data.contains("command") && !data["command"].is_null()) {
    //         return receive_message(timeout_ms);
    //     }

    //     return json{};
    // }

void WayfireIPC::send_message(const std::string& message, int timeout_ms)
{
    size_t sent = 0;
    while (sent < message.size()) {
        struct pollfd pfd = {
            .fd = socket_fd,
            .events = POLLOUT,
            .revents = 0
        };

        int ready = poll(&pfd, 1, timeout_ms);
        if (ready == 0) {
            throw std::runtime_error("Send operation timed out");
        }
        if (ready == -1) {
            throw std::system_error(errno, std::system_category(), "poll failed");
        }

        std::cout<<"sending..."<<std::endl;
        uint32_t length = message.size();
        auto all_data = std::string((char*)&length, 4);
        // std::cout<<"sending size: "<<all_data.data()<<std::endl;
        all_data += message;
        ssize_t res = ::send(socket_fd, all_data.data() + sent, 
                            all_data.size() - sent, MSG_NOSIGNAL);
        if (res == -1) {
            throw std::system_error(errno, std::system_category(), "send failed");
        }

        sent += res;
        std::cout<<"sent bytes: "<<sent<<std::endl;
    }
    std::cout<<"sent all"<<std::endl;
}

std::string WayfireIPC::receive_message(int timeout_ms)
{
    std::string buffer;
    struct pollfd pfd = {
        .fd = socket_fd,
        .events = POLLIN,
        .revents = 0
    };

    while (true) {
        int ready = poll(&pfd, 1, timeout_ms);
        if (ready == 0) {
            throw std::runtime_error("Receive operation timed out");
        }
        if (ready == -1) {
            throw std::system_error(errno, std::system_category(), "poll failed");
        }

        uint32_t length;
        ssize_t received = ::recv(socket_fd, &length, 4, 0);
        if (received == -1) {
            throw std::system_error(errno, std::system_category(), "recv failed");
        }
        if (received == 0) {
            throw std::runtime_error("Connection closed by peer");
        }

        std::vector<char> buf(length + 1, 0);
        received = ::recv(socket_fd, &buf[0], length, 0);
        if (received == -1) {
            throw std::system_error(errno, std::system_category(), "recv failed");
        }
        if (received == 0) {
            throw std::runtime_error("Connection closed by peer");
        }

        auto message = std::string(buf.data(), length);
        // buffer.append(chunk, received);
        
        // Сообщение завершается символом новой строки
        // size_t end_pos = buffer.find('\n');
        // if (end_pos != std::string::npos) {
        //     std::string message = buffer.substr(0, end_pos);
        //     buffer.erase(0, end_pos + 1);
            
            // try {
                // return json::parse(message);
                return message;
            // } catch (const json::parse_error& e) {
            //     throw std::runtime_error("JSON parse error: " + std::string(e.what()));
            // }
        // }
    }
}
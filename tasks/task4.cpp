#include <iostream>
#include <array>
#include <boost/asio.hpp>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>

using boost::asio::ip::tcp;

void task4_server() {
    boost::asio::io_context io;

    // 1. listen on port 8888
    tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 8888));
    std::cout << "Listening on 8888...\n";

    // 2. wait for connection
    tcp::socket socket(io);
    acceptor.accept(socket);
    std::cout << "Client connected!\n";

    // 3. receive data
    std::array<char, 1024> buf;

    while (true) {
        boost::system::error_code ec;
        size_t len = socket.read_some(boost::asio::buffer(buf), ec);

        if (ec) break;

        std::cout << "Received: " << std::string(buf.data(), len);
    }

    std::cout << "Client disconnected.\n";
}



void task4_client() {
    boost::asio::io_context io;

    // 1. create socket
    tcp::socket socket(io);

    // 2. connect to server
    tcp::endpoint endpoint(
        boost::asio::ip::make_address("127.0.0.1"),
        8888
    );

    socket.connect(endpoint);
    std::cout << "Connected!\n";

    // 3. send messages
    while (true) {
        std::string msg;
        std::getline(std::cin, msg);

        msg += "\n";

        boost::asio::write(
            socket,
            boost::asio::buffer(msg)
        );
    }
}




std::vector<std::shared_ptr<tcp::socket>> clients;
std::mutex clients_mutex;

void task4_server_chatroom() {
    boost::asio::io_context io;
    tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 8888));

    std::cout << "Chat server on 8888...\n";

    while (true) {
        auto socket = std::make_shared<tcp::socket>(io);
        acceptor.accept(*socket);

        std::cout << "New client connected\n";

        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.push_back(socket);

        std::thread([socket]() {
            std::array<char, 1024> buf;

            while (true) {
                boost::system::error_code ec;
                size_t len = socket->read_some(boost::asio::buffer(buf), ec);

                if (ec) break;

                std::string msg(buf.data(), len);
                std::cout << "Received: " << msg;

                std::lock_guard<std::mutex> lock(clients_mutex);
                for (auto& client : clients) {
                    if (client != socket) {
                        boost::asio::write(*client, boost::asio::buffer(msg), ec);
                    }
                }
            }

            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.erase(
                std::remove(clients.begin(), clients.end(), socket),
                clients.end()
            );

            std::cout << "Client disconnected\n";
        }).detach();
    }
}
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <limits>
#include <nlohmann/json.hpp>

using boost::asio::ip::tcp;
using json = nlohmann::json;

void print_board(const json& board) {
    std::cout << "    0   1   2\n";
    std::cout << "  +---+---+---+\n";
    for (int r = 0; r < 3; ++r) {
        std::cout << r << " | "
                  << board[r][0].get<std::string>() << " | "
                  << board[r][1].get<std::string>() << " | "
                  << board[r][2].get<std::string>() << " |\n";
        std::cout << "  +---+---+---+\n";
    }
}

void print_legal_moves(const json& moves) {
    std::cout << "Legal moves: ";
    for (const auto& mv : moves) {
        std::cout << "(" << mv[0].get<int>() << "," << mv[1].get<int>() << ") ";
    }
    std::cout << "\n";
}

void task6_client() {
    boost::asio::io_context io;
    tcp::socket socket(io);

    tcp::endpoint endpoint(
        boost::asio::ip::make_address("127.0.0.1"),
        9999
    );

    boost::system::error_code ec;
    socket.connect(endpoint, ec);
    if (ec) {
        std::cout << "Connect failed: " << ec.message() << "\n";
        return;
    }

    std::cout << "Connected to TicTacToe server.\n";

    boost::asio::streambuf buf;
    while (true) {
        boost::asio::read_until(socket, buf, '\n', ec);
        if (ec) { std::cout << "Connection closed: " << ec.message() << "\n"; break;}
        std::istream is(&buf);
        std::string line;
        std::getline(is, line);
        if (line.empty()) continue;
        json msg;
        try {
            msg = json::parse(line);
        } catch (...) { std::cout << "Received invalid JSON: " << line << "\n"; continue; }
        // TODO 1: Convert strings to JSON format

        std::string type = msg.value("type", "");

        if (type == "welcome") {
            std::cout << "Welcome: " << msg.value("you", "")
                      << " (" << msg.value("symbol", "") << ")\n";
        }
        else if (type == "state") {
            std::cout << msg.value("message", "") << "\n";
            print_board(msg["board"]);
            print_legal_moves(msg["legal_moves"]);
        }
        else if (type == "your_turn") {
            std::cout << msg.value("message", "Your turn") << "\n";
            print_board(msg["board"]);
            print_legal_moves(msg["legal_moves"]);
            int row, col;
            std::cout << "Enter move (row col): ";
            std::cin >> row >> col;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            json out = { {"type", "move"}, {"row", row}, {"col", col} };
            std::string s = out.dump() + "\n";
            boost::asio::write(socket, boost::asio::buffer(s), ec);
            if (ec) {std::cout << "Write failed: " << ec.message() << "\n"; break; }
        }

        else if (type == "error") {
            std::cout << "Error: " << msg.value("message", "") << "\n";
        }
        else if (type == "info") {
            std::cout << msg.value("message", "") << "\n";
        }
        else if (type == "game_over") {
            std::cout << "Game over.\n";
            std::cout << msg.dump(2) << "\n";
            break;
        }
        else {
            std::cout << "Unknown message: " << msg.dump() << "\n";
        }
    }
}
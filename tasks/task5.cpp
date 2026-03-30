#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>
#include <librealsense2/rs.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

void task5_server() {
    boost::asio::io_context io;
    tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 8888));

    std::cout << "Server listening on 8888...\n";

    tcp::socket socket(io);
    acceptor.accept(socket);

    std::cout << "Client connected!\n";

    while (true) {
        int size = 0;
        boost::asio::read(socket, boost::asio::buffer(&size, sizeof(size)));

        std::vector<uchar> buffer(size);
        boost::asio::read(socket, boost::asio::buffer(buffer.data(), size));

        cv::Mat img = cv::imdecode(buffer, cv::IMREAD_COLOR);
        if (img.empty()) continue;

        cv::imshow("Server View", img);

        if (cv::waitKey(1) == 27) break;
    }
}

void task5_client() {
    boost::asio::io_context io;
    tcp::socket socket(io);

    tcp::endpoint endpoint(
        boost::asio::ip::make_address("127.0.0.1"),
        8888
    );

    socket.connect(endpoint);
    std::cout << "Connected to server!\n";

    rs2::pipeline pipe;
    rs2::config cfg;

    cfg.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_BGR8, 30);
    pipe.start(cfg);

    while (true) {
        rs2::frameset frames = pipe.wait_for_frames();
        rs2::video_frame color_frame = frames.get_color_frame();

        if (!color_frame) continue;

        cv::Mat img(
            cv::Size(color_frame.get_width(), color_frame.get_height()),
            CV_8UC3,
            (void*)color_frame.get_data(),
            cv::Mat::AUTO_STEP
        );

        std::vector<uchar> buffer;
        cv::imencode(".jpg", img, buffer);

        int size = (int)buffer.size();

        boost::asio::write(socket, boost::asio::buffer(&size, sizeof(size)));
        boost::asio::write(socket, boost::asio::buffer(buffer.data(), size));

        cv::imshow("Client View", img);

        if (cv::waitKey(1) == 27) break;
    }
}
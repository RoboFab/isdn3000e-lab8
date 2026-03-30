#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <librealsense2/rs.hpp>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>

void task3() {
    std::string model_path = "data/models/yolov8n.onnx";
    std::string class_path = "data/models/coco.txt";

    std::vector<std::string> class_names;
    std::ifstream ifs(class_path);
    std::string line;
    while (std::getline(ifs, line)) {
        if (!line.empty()) {
            class_names.push_back(line);
        }
    }

    // TODO 1: Load the YOLO network model via cv::dnn::readNet().
    cv::dnn::Net net = cv::dnn::readNet(model_path);

    rs2::pipeline pipe;
    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_COLOR, 640, 640, RS2_FORMAT_BGR8, 30);
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

        cv::Mat frame = img.clone();

        cv::Mat input;

        // TODO 2: Convert the image into the format compatible with YOLO input.
        cv::resize(frame, input, cv::Size(640, 640));
        cv::Mat blob = cv::dnn::blobFromImage(
            input,
            1.0 / 255.0,
            cv::Size(640, 640),
            cv::Scalar(),
            true,
            false
        );

        std::vector<cv::Mat> outputs;
        // TODO 3: Run the model inference.
        net.setInput(blob);
        net.forward(outputs, net.getUnconnectedOutLayersNames());


        if (outputs.empty()) {
            cv::imshow("YOLO Detection", frame);
            int key = cv::waitKey(1);
            if (key == 27) break;
            continue;
        }

        cv::Mat output = outputs[0];

        if (output.dims == 3) {
            int channels = output.size[1];
            int num_boxes = output.size[2];
            (void)num_boxes;
            output = output.reshape(1, channels);
            cv::transpose(output, output);
        }

        std::vector<int> class_ids;
        std::vector<float> scores;
        std::vector<cv::Rect> boxes;

        float x_factor = static_cast<float>(frame.cols) / 640.0f;
        float y_factor = static_cast<float>(frame.rows) / 640.0f;

        for (int i = 0; i < output.rows; i++) {
            float* data = (float*)output.ptr(i);

            float cx = data[0];
            float cy = data[1];
            float w  = data[2];
            float h  = data[3];

            cv::Mat scores_mat(1, output.cols - 4, CV_32F, data + 4);
            cv::Point class_id_point;
            double max_class_score;
            cv::minMaxLoc(scores_mat, 0, &max_class_score, 0, &class_id_point);

            float confidence = static_cast<float>(max_class_score);
            if (confidence < 0.25f) continue;

            int left   = static_cast<int>((cx - 0.5f * w) * x_factor);
            int top    = static_cast<int>((cy - 0.5f * h) * y_factor);
            int width  = static_cast<int>(w * x_factor);
            int height = static_cast<int>(h * y_factor);

            cv::Rect box(left, top, width, height);
            box &= cv::Rect(0, 0, frame.cols, frame.rows);

            if (box.width <= 0 || box.height <= 0) continue;

            class_ids.push_back(class_id_point.x);
            scores.push_back(confidence);
            boxes.push_back(box);
        }

        std::vector<int> indices;
        cv::dnn::NMSBoxes(boxes, scores, 0.25f, 0.45f, indices);

        cv::Mat result = frame.clone();

        for (int idx : indices) {
            cv::Rect box = boxes[idx];
            cv::rectangle(result, box, cv::Scalar(0, 255, 0), 2);

            std::string label;
            if (class_ids[idx] >= 0 && class_ids[idx] < (int)class_names.size()) {
                label = class_names[class_ids[idx]] + " " + cv::format("%.2f", scores[idx]);
            } else {
                label = "id " + std::to_string(class_ids[idx]) + " " + cv::format("%.2f", scores[idx]);
            }

            cv::putText(
                result,
                label,
                cv::Point(box.x, std::max(box.y - 10, 20)),
                cv::FONT_HERSHEY_SIMPLEX,
                0.6,
                cv::Scalar(0, 255, 0),
                2
            );
        }

        cv::imshow("YOLO Detection", result);

        int key = cv::waitKey(1);
        if (key == 27) break;
    }

    cv::destroyAllWindows();
}
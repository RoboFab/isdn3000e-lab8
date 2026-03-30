#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>
#include <librealsense2/rs.hpp>
#include <vector>

void task2() {

    // TODO 1: Initialize the camera configuration.
    rs2::pipeline pipe;
    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_COLOR, 1280, 720, RS2_FORMAT_BGR8, 30);
    rs2::pipeline_profile profile = pipe.start(cfg);

    rs2::video_stream_profile color_profile = profile.get_stream(RS2_STREAM_COLOR).as<rs2::video_stream_profile>();
    rs2_intrinsics intr = color_profile.get_intrinsics();
    cv::Mat camera_matrix = (cv::Mat_<double>(3, 3) <<
        intr.fx, 0,       intr.ppx,
        0,       intr.fy, intr.ppy,
        0,       0,       1);
    cv::Mat dist_coeffs = cv::Mat::zeros(1, 5, CV_64F);

    // TODO 2: Initialize the ArUco detector.
    //  - 1) define the marker dictionary
    //  - 2) create the detector
    auto dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_50);
    cv::aruco::DetectorParameters detector_params;
    cv::aruco::ArucoDetector detector(dictionary, detector_params);

    const float marker_length = 0.05f;
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
        cv::Mat display = img.clone();

        std::vector<int> marker_ids;
        std::vector<std::vector<cv::Point2f>> marker_corners;
        std::vector<std::vector<cv::Point2f>> rejected;

        // TODO 3: Use detector you created to detect the marker via detector.detectMarkers().
        detector.detectMarkers(display, marker_corners, marker_ids, rejected);

    if (!marker_ids.empty()) {
        std::vector<cv::Vec3d> rvecs, tvecs;
        // TODO 4: Draw detected markers via cv::aruco::drawDetectedMarkers() and estimate the marker pose via cv::aruco::estimatePoseSingleMarkers().
        cv::aruco::drawDetectedMarkers(display, marker_corners, marker_ids);
        cv::aruco::estimatePoseSingleMarkers(
            marker_corners,
            marker_length,
            camera_matrix,
            dist_coeffs,
            rvecs,
            tvecs
        );

        for (size_t i = 0; i < marker_ids.size(); ++i) {
            cv::drawFrameAxes(display, camera_matrix, dist_coeffs, rvecs[i], tvecs[i], 0.04f);

            std::ostringstream text;
            text << "ID: " << marker_ids[i]
                 << "  t = ["
                 << std::fixed << std::setprecision(2)
                 << tvecs[i][0] << ", "
                 << tvecs[i][1] << ", "
                 << tvecs[i][2] << "] m";

            cv::Point text_pos(
                static_cast<int>(marker_corners[i][0].x),
                static_cast<int>(marker_corners[i][0].y) - 10
            );

            cv::putText(
                display,
                text.str(),
                text_pos,
                cv::FONT_HERSHEY_SIMPLEX,
                0.5,
                cv::Scalar(0, 255, 0),
                2
            );
        }
    }

    cv::imshow("ArUco Pose", display);

    int key = cv::waitKey(1);
    if (key == 27) break;
}

    cv::destroyAllWindows();
}
#include <iostream>
#include <opencv2/opencv.hpp>
#include "image_sensor.hpp"

namespace mc { // My Camera
    ImageSensor::ImageSensor() {
        this->capture_ = cv::VideoCapture(0, cv::CAP_V4L2);
        this->capture_.set(cv::CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
        this->capture_.set(cv::CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);
        this->capture_.set(cv::CAP_PROP_FPS, FPS);
        this->capture_.set(cv::CAP_PROP_BUFFERSIZE, FRAME_BUFFER_SIZE);

        assert(this->capture_.isOpened());

        // TODO: consider to use logging library
        std::cout << "cv::VideoCapture width: "
                  << this->capture_.get(cv::CAP_PROP_FRAME_WIDTH)
                  << std::endl;
        std::cout << "cv::VideoCapture height: "
                  << this->capture_.get(cv::CAP_PROP_FRAME_HEIGHT)
                  << std::endl;
        std::cout << "cv::VideoCapture fps: "
                  << this->capture_.get(cv::CAP_PROP_FPS)
                  << std::endl;
        std::cout << "cv::VideoCapture buffer size: "
                  << this->capture_.get(cv::CAP_PROP_BUFFERSIZE)
                  << std::endl;
        std::cout << "cv::VideoCapture format: "
                  << this->capture_.get(cv::CAP_PROP_FORMAT)
                  << std::endl;
    }

    ImageSensor::~ImageSensor() {}

    cv::Mat ImageSensor::getFrame() {
        cv::Mat frame;
        this->capture_ >> frame;
        return frame;
    }
}

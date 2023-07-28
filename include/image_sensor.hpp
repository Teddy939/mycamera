#pragma once

namespace mc { // My Camera
    class ImageSensor {
        public:
            ImageSensor();
            ~ImageSensor();
            cv::Mat getFrame();
            static constexpr int16_t FRAME_WIDTH = 1280; // 640
            static constexpr int16_t FRAME_HEIGHT = 720; // 360
            static constexpr int16_t FPS = 30;
            static constexpr int16_t FRAME_BUFFER_SIZE = 4;
        private:
            cv::VideoCapture capture_;
    };
}


#pragma once

namespace mc { // My Camera
    class LiveViewer {
        public:
            LiveViewer(const Focuser &focuser);
            ~LiveViewer();
            void show(cv::Mat &frame);
        private:
            static constexpr int16_t const LINE_WIDTH = 2;
            const Focuser &focuser_;
            void drawFocusRoi_(cv::Mat &frame, const cv::Scalar lineColor);
    };
}

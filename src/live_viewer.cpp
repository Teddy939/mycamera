#include <iostream>
#include <opencv2/opencv.hpp>
#include "focuser.hpp"
#include "live_viewer.hpp"

namespace mc { // My Camera
    LiveViewer::LiveViewer(const Focuser &focuser)
        : focuser_(focuser)
    {}

    LiveViewer::~LiveViewer() {}

    void LiveViewer::show(cv::Mat &frame) {
        if (this->focuser_.state() == Focuser::State::Idle
            || this->focuser_.state() == Focuser::State::Manual) {
            drawFocusRoi_(frame, cv::Scalar(255, 0, 0)); // blue
        }
        else if (this->focuser_.state() == Focuser::State::Succeeded) {
            drawFocusRoi_(frame, cv::Scalar(0, 255, 0)); // green
        }
        else if (this->focuser_.state() == Focuser::State::Failed) {
            drawFocusRoi_(frame, cv::Scalar(0, 0, 255)); // red
        }
        else {
            drawFocusRoi_(frame, cv::Scalar(127, 127, 127)); // gray
        }

        cv::imshow("LiveView", frame);
    }

    void LiveViewer::drawFocusRoi_(cv::Mat &frame, const cv::Scalar lineColor) {
        auto roi = this->focuser_.roi();
        cv::rectangle(
            frame,
            cv::Point(roi.x, roi.y),
            cv::Point(roi.x + roi.width, roi.y + roi.height),
            lineColor,
            LINE_WIDTH
        );
    }
}

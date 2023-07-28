#include <iostream>
#include <opencv2/opencv.hpp>
#include "image_sensor.hpp"
#include "focuser.hpp"
#include "live_viewer.hpp"

constexpr int16_t FOCUS_ROI_SIZE = 120;

void control(
    mc::Focuser &focuser,
    const cv::Mat &frame,
    bool &quit
);

int main() {
    mc::ImageSensor sensor;
    mc::Focuser focuser;
    mc::LiveViewer liveViewer(focuser);

    auto focusRoi = cv::Rect(
        mc::ImageSensor::FRAME_WIDTH / 2 - FOCUS_ROI_SIZE / 2,
        mc::ImageSensor::FRAME_HEIGHT / 2 - FOCUS_ROI_SIZE / 2,
        FOCUS_ROI_SIZE,
        FOCUS_ROI_SIZE
    ); // image centor

    focuser.setRoi(focusRoi);

    bool quit = false;
    while (!quit) { // main loop
        auto frame = sensor.getFrame();
        assert(!frame.empty());

        focuser.update(frame);

        liveViewer.show(frame);

        control(focuser, frame, quit);
        // TODO: consider to make Controller class
    }

    return 0;
}

void control(
    mc::Focuser &focuser,
    const cv::Mat &frame,
    bool &quit
) {
    auto key = cv::waitKey(1000.0 /  mc::ImageSensor::FPS);
    // std::cout << "key: " << key << std::endl;
    if (key == 102) { // "f"
        focuser.start(frame);
    }
    if (key == 82) { // arrow up
        focuser.far();
    }
    if (key == 84) { // arrow down
        focuser.near();
    }
    if (key == 113) { // "q"
        quit = true;
    }
}

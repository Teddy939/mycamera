#include <iostream>
#include <cmath>
#include <chrono>
#include <opencv2/opencv.hpp>
extern "C" {
    #include "arducam_vcm.h"
}
#include "focuser.hpp"

namespace mc { // My Camera
    Focuser::Focuser()
        : state_(Focuser::InnerState::Idle),
          currentPosition_(INITIAL_POSITION),
          maxContrastPosition_(0),
          maxContrastValue_(0.0),
          fineScanStartPosition_(0),
          roi_(cv::Rect(0, 0, 0, 0)),
          targetPosition_(0)
    {
        auto vcmInitStatus = vcm_init();
        std::cout << "vcm init status: "
                  << vcmInitStatus
                  << std::endl; // TODO: use logging library

        startMovingFocusPositionTo_(INITIAL_POSITION);
    }

    Focuser::~Focuser() {}

    void Focuser::setRoi(const cv::Rect roi) {
        this->roi_ = roi;
    }

    bool Focuser::isSetRoi_() {
        if (0 == this->roi_.width * this->roi_.height) {
            return false;
        }
        else {
            return true;
        }
    }

    // TODO: consider to run the focusing task in a thread
    void Focuser::start(const cv::Mat &currentFrame) {
        if (!isSetRoi_()) {
            std::cout << "ROI is not set" << std::endl;
            return;
        }

        std::cout << "start autofocus sequence" << std::endl;

        startMovingFocusPositionTo_(MINIMUM_POSITION);
        this->state_ = Focuser::InnerState::StartCoarseScan;
    }

    void Focuser::update(const cv::Mat &currentFrame) {
        if (this->state_ == Focuser::InnerState::Idle) {
            return;
        }

        auto now = std::chrono::system_clock::now();
        auto elapsedInMillisec = std::chrono::duration_cast<std::chrono::milliseconds>(now - this->lastTimeMoved_).count();
        if (elapsedInMillisec < MOVING_INTERVAL_IN_MILLISEC) {
            return;
        }

        if (this->currentPosition_ != this->targetPosition_) {
            // continueMovingFocusPosition_();
            moveFocus_();
            return;
        }

        // TODO: consider to use state pattern
        if (this->state_ == Focuser::InnerState::StartCoarseScan) {
            startCoarseScan_(contrast_(this->roi_, currentFrame));
        }
        else if (this->state_ == Focuser::InnerState::CoarseScanning) {
            scanCoarse_(contrast_(this->roi_, currentFrame));
        }
        else if (this->state_ == Focuser::InnerState::StartFineScan) {
            startFineScan_();
        }
        else if (this->state_ == Focuser::InnerState::FineScanning) {
            scanFine_(contrast_(this->roi_, currentFrame));
        }
        else if (this->state_ == Focuser::InnerState::GoingBackToPeak) {
            goBackToPeak_();
        }
        else if (this->state_ == Focuser::InnerState::Manual) {
            std::cout << "manual" << std::endl;
            auto c = contrast_(this->roi_, currentFrame);
            this->state_ = Focuser::InnerState::Idle;
        }
        else { // Succeeded or Failed
            moveStateToIdleAfter_(
                STATE_KEEPING_DULATION_IN_MILLISEC,
                elapsedInMillisec
            );
        }

        std::cout << "position: " <<  this->currentPosition_ << std::endl;
    }

    Focuser::State Focuser::state() const {
        // TODO: consider to implement state pattern
        // to avoid switch statements 
        if (this->state_ == Focuser::InnerState::Idle) {
            return Focuser::State::Idle;
        }
        else if (this->state_ == Focuser::InnerState::Succeeded) {
            return Focuser::State::Succeeded;
        }
        else if (this->state_ == Focuser::InnerState::Failed) {
            return Focuser::State::Failed;
        }
        else if (this->state_ == Focuser::InnerState::Manual) {
            return Focuser::State::Manual;
        }
        else {
            return Focuser::State::Moving;
        }
    }

    cv::Rect Focuser::roi() const {
        return this->roi_;
    }

    void Focuser::near() {
        this->targetPosition_ = (
            MAXIMUM_POSITION
            < this->currentPosition_ + POSITION_STEP_MANUAL
        ) ? MAXIMUM_POSITION :  this->currentPosition_ + POSITION_STEP_MANUAL;
        std::cout << "go near to: " << this->targetPosition_ << std::endl;

        moveFocusTo_(this->targetPosition_);
        this->state_ = Focuser::InnerState::Manual;
    }

    void Focuser::far() {
        this->targetPosition_ = (
            MINIMUM_POSITION
            > this->currentPosition_ - POSITION_STEP_MANUAL
        ) ? MINIMUM_POSITION : this->currentPosition_ - POSITION_STEP_MANUAL;
        std::cout << "go far to: " << this->targetPosition_ << std::endl;

        moveFocusTo_(this->targetPosition_);
        this->state_ = Focuser::InnerState::Manual;
    }

    double Focuser::contrast_(const cv::Rect roi, const cv::Mat &image) {
        auto roiImage = image(roi);

        cv::Mat roiImageGray;
        cv::cvtColor(roiImage, roiImageGray, cv::COLOR_RGB2GRAY);

        cv::Mat mean, stdDev;
        cv::meanStdDev(roiImageGray, mean, stdDev);
        std::cout << "contrast value: "
                  << stdDev.at<double>(0, 0)
                  << std::endl; // TODO: use logging library

        return stdDev.at<double>(0, 0);
    }

    void Focuser::startMovingFocusPositionTo_(int16_t targetPosition) {
        std::cout << "target position: " << targetPosition << std::endl;

        auto targetPositionSaturated = targetPosition;
        if (targetPositionSaturated < MINIMUM_POSITION) {
            std::cout << "saturated to " << MINIMUM_POSITION << std::endl;
            targetPositionSaturated = MINIMUM_POSITION;
        }
        if (targetPositionSaturated > MAXIMUM_POSITION) {
            std::cout << "saturated to " << MAXIMUM_POSITION << std::endl;
            targetPositionSaturated = MAXIMUM_POSITION;
        }

        this->targetPosition_ = targetPositionSaturated;
        moveFocus_();
    }

    void Focuser::moveFocus_() {
        auto distance = this->targetPosition_ - this->currentPosition_;
        std::cout << "distance: " << distance << std::endl;

        if (std::abs(distance) > POSITION_STEP_FINE * STEP_NUMBER_GENTLE_MOVE) {
            moveFocusFast_(distance);
        }
        else {
            moveFocusGently_(distance);
        }
    }

    void Focuser::moveFocusFast_(int16_t distance) {
        auto targetPositionOffset = POSITION_STEP_FINE * STEP_NUMBER_GENTLE_MOVE;
        if (distance > 0) {
            targetPositionOffset *= -1;
        }

        auto targetPositionForFastMoving = this->targetPosition_ + targetPositionOffset;
        auto distanceForFastMoving = targetPositionForFastMoving - this->currentPosition_;

        int16_t position = 0;
        if (std::abs(distanceForFastMoving) > MAXIMUM_MOVING_DISTANCE) {
            if (distanceForFastMoving < 0) {
                position = this->currentPosition_ - MAXIMUM_MOVING_DISTANCE;
            }
            else {
                position = this->currentPosition_ + MAXIMUM_MOVING_DISTANCE;
            }
        }
        else {
            position = targetPositionForFastMoving;
        }

        moveFocusTo_(position);
    }

    void Focuser::moveFocusGently_(int16_t distance) {
        int16_t position = 0;

        if (std::abs(distance) > POSITION_STEP_FINE) {
            if (distance < 0) {
                position = this->currentPosition_ - POSITION_STEP_FINE;
            }
            else {
                position = this->currentPosition_ + POSITION_STEP_FINE;
            }
        }
        else {
            position = this->targetPosition_;
        }

        moveFocusTo_(position);
    }

    void Focuser::moveFocusTo_(int16_t position) {
        this->currentPosition_ = position;
        vcm_write(this->currentPosition_);
        this->lastTimeMoved_ = std::chrono::system_clock::now();
    }

    void Focuser::startCoarseScan_(double currentContrast) {
        std::cout << "startCoarseScan" << std::endl;

        this->maxContrastValue_ = currentContrast;
        this->maxContrastPosition_ = this->currentPosition_;
        this->state_ = Focuser::InnerState::CoarseScanning;
    }

    void Focuser::scanCoarse_(double currentContrast) {
        std::cout << "scanCoarse" << std::endl;

        if (currentContrast > this->maxContrastValue_) {
            this->maxContrastValue_ = currentContrast;
            this->maxContrastPosition_ = this->currentPosition_;
        }

        if (this->currentPosition_ == MAXIMUM_POSITION) {
            if (this->maxContrastPosition_ == MAXIMUM_POSITION) {
                // max contrast at MINIMUN_POSITION is ok
                // because it means that focus point is at infinite far.
                this->state_ = Focuser::InnerState::Failed;
            }
            else {
                this->state_ = Focuser::InnerState::StartFineScan;
            }

            return;
        }

        this->targetPosition_ = (
            MAXIMUM_POSITION
            < this->currentPosition_ + POSITION_STEP_COARSE
        ) ? MAXIMUM_POSITION : this->currentPosition_ + POSITION_STEP_COARSE;
        moveFocusTo_(this->targetPosition_);
    }

    void Focuser::startFineScan_() {
        std::cout << "startFineScan" << std::endl;

        this->fineScanStartPosition_ = this->maxContrastPosition_ + FINE_SCAN_POSITION_RANGE / 2;
        this->maxContrastValue_ = 0;
        startMovingFocusPositionTo_(this->fineScanStartPosition_);

        this->state_ = Focuser::InnerState::FineScanning;
    }

    void Focuser::scanFine_(double currentContrast) {
        std::cout << "scanFine" << std::endl;

        if (currentContrast > this->maxContrastValue_) {
            this->maxContrastValue_ = currentContrast;
            this->maxContrastPosition_ = this->currentPosition_;
        }

        auto fineScanEndPosition = (
            MINIMUM_POSITION
            > this->fineScanStartPosition_ - FINE_SCAN_POSITION_RANGE
        ) ? MINIMUM_POSITION : this->fineScanStartPosition_ - FINE_SCAN_POSITION_RANGE;

        if (this->currentPosition_ <= fineScanEndPosition) {
            this->state_ = Focuser::InnerState::GoingBackToPeak;

            return;
        }

        this->targetPosition_ = (
            MINIMUM_POSITION
            > this->currentPosition_ - POSITION_STEP_FINE
        ) ? MINIMUM_POSITION : this->currentPosition_ - POSITION_STEP_FINE;
        moveFocusTo_(this->targetPosition_);
    }

    void Focuser::goBackToPeak_() {
        std::cout << "goBackToPeak" << std::endl;

        this->targetPosition_ = this->maxContrastPosition_;
        moveFocusTo_(this->targetPosition_);
        this->state_ = Focuser::InnerState::Succeeded;
    }

    void Focuser::moveStateToIdleAfter_(double durationInMillisec, double elapsedInMillisec) {
        std::cout << "moveStateToIdleAfter" << std::endl;

        if (elapsedInMillisec > durationInMillisec) {
            this->state_ = Focuser::InnerState::Idle;
        }
    }
}

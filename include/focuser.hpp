#pragma once

namespace mc { // My Camera
    class Focuser {
        public:
            Focuser();
            ~Focuser();
            void setRoi(const cv::Rect roi);
            void start(const cv::Mat &currentFrame);
            void update(const cv::Mat &currentFrame);
            enum class State {
                Idle,
                Moving,
                Succeeded,
                Failed,
                Manual
            };
            State state() const;
            cv::Rect roi() const;
            void near(); // manual
            void far(); // manual
        private:
            static constexpr int16_t const INITIAL_POSITION = 240;
            static constexpr int16_t const MINIMUM_POSITION = 240;
            static constexpr int16_t const MAXIMUM_POSITION = 900;
            static constexpr int16_t const MAXIMUM_MOVING_DISTANCE = 100;
            static constexpr int16_t const STEP_NUMBER_GENTLE_MOVE = 3;
            static constexpr int16_t const POSITION_STEP_COARSE = 30;
            static constexpr int16_t const POSITION_STEP_FINE = 10;
            static constexpr int16_t const POSITION_STEP_MANUAL = 10;
            static constexpr int16_t const FINE_SCAN_POSITION_RANGE = 100;
            static constexpr double const MOVING_INTERVAL_IN_MILLISEC = 66;
            static constexpr double const STATE_KEEPING_DULATION_IN_MILLISEC = 1000;

            enum class InnerState {
                Idle,
                StartCoarseScan,
                CoarseScanning,
                StartFineScan,
                FineScanning,
                GoingBackToPeak,
                Succeeded,
                Failed,
                Manual
            };

            InnerState state_;
            int16_t currentPosition_;
            int16_t maxContrastPosition_;
            double maxContrastValue_;
            int16_t fineScanStartPosition_;
            cv::Rect roi_;
            std::chrono::system_clock::time_point lastTimeMoved_;

            bool isSetRoi_();
            double contrast_(const cv::Rect roi, const cv::Mat &image);

            int16_t targetPosition_;
            void startMovingFocusPositionTo_(int16_t targetPosition);
            void moveFocus_();
            void moveFocusFast_(int16_t distance);
            void moveFocusGently_(int16_t distance);
            void moveFocusTo_(int16_t position);

            void startCoarseScan_(double currentContrast);
            void scanCoarse_(double currentContrast);
            void startFineScan_();
            void scanFine_(double currentContrast);
            void goBackToPeak_();
            void moveStateToIdleAfter_(double durationInMillisec, double elapsedInMillisec);
    };
}

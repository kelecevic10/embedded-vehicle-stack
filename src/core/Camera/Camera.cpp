//
// Created by nemanja on 4/23/26.
//

#include "Camera.h"

#include "utils/logger/Logger.h"

Camera::~Camera() {
    if (running_.load()) {
        Camera::stop();
    }
}

// NOTE: Those handlers will be executed by the listener thread
// to make them easy & lightweight, their job is to signal the worker thread
// that some job arrived and thread-safely wake them up to work
void Camera::registerHandlers() {
    subscribe(EventType::CAMERA_TAKE_SNAPSHOT, [this](const Event&) {
        std::scoped_lock lock(cameraMutex_);
        takeSnapShot_ = true;
        workerCv_.notify_one();
    });

    subscribe(EventType::CAMERA_START_PREVIEW, [this](const Event&) {
        std::scoped_lock lock(cameraMutex_);
        requestedState_ = CameraState::Previewing;
        workerCv_.notify_one();
    });

    subscribe(EventType::CAMERA_STOP_PREVIEW, [this](const Event&) {
        std::scoped_lock lock(cameraMutex_);
        requestedState_ = CameraState::Idle;
        workerCv_.notify_one();
    });

    subscribe(EventType::CAMERA_START_RECORDING, [this](const Event&) {
        std::scoped_lock lock(cameraMutex_);
        requestedState_ = CameraState::Recording;
        workerCv_.notify_one();
    });

    subscribe(EventType::CAMERA_STOP_RECORDING, [this](const Event&) {
        std::scoped_lock lock(cameraMutex_);
        requestedState_ = CameraState::Idle;
        workerCv_.notify_one();
    });
}

bool Camera::start() {
    if (running_.load()) {
        Logger::warning("Camera already running!");
        return false;
    }

    {
        std::lock_guard lock(cameraMutex_);
        updateState(CameraState::Initializing);
        requestedState_ = CameraState::Idle;
    }

    backend_.setFrameCallback([this](const FrameData& frame) {
        if (frame.sequenceNumber % 30 != 0) {
            return;
        }

        Logger::info(
            std::format(
           "Camera frame received! {}: {}",
                   frame.sequenceNumber,
                   frame.timestampNs
                   ));
    });

    backend_.setErrorCallback([this](const std::string& error) {
        Logger::error(std::format("Camera error: {}", error));
        std::scoped_lock lock(cameraMutex_);
        updateState(CameraState::Faulted);
        requestedState_ = CameraState::Faulted;
    });

    if (!backend_.initialize()) {
        std::scoped_lock lock(cameraMutex_);
        updateState(CameraState::Faulted);
        requestedState_ = CameraState::Faulted;
        return false;
    }

    {
        std::lock_guard lock(cameraMutex_);
        updateState(CameraState::Idle);
        requestedState_ = CameraState::Idle;
    }

    // start the worker thread
    running_.store(true);
    workerThread_ = std::jthread(&Camera::workerLoop, this);

    // start the listener thread
    Camera::registerHandlers();
    startEventLoop();

    bus_->subscribe(EventType::CAMERA_TAKE_SNAPSHOT, this);
    bus_->subscribe(EventType::CAMERA_START_RECORDING, this);
    bus_->subscribe(EventType::CAMERA_STOP_RECORDING, this);
    bus_->subscribe(EventType::CAMERA_START_PREVIEW, this);
    bus_->subscribe(EventType::CAMERA_STOP_PREVIEW, this);

    return true;
}

bool Camera::stop() {
    if (!running_.load()) {
        Logger::warning("Camera already stopped!");
        return true;
    }

    {
        std::lock_guard lock(cameraMutex_);
        updateState(CameraState::ShuttingDown);
        requestedState_ = CameraState::ShuttingDown;
    }

    stopEventLoop();
    bus_->unsubscribe(EventType::CAMERA_TAKE_SNAPSHOT, this);
    bus_->unsubscribe(EventType::CAMERA_START_RECORDING, this);
    bus_->unsubscribe(EventType::CAMERA_STOP_RECORDING, this);
    bus_->unsubscribe(EventType::CAMERA_START_PREVIEW, this);
    bus_->unsubscribe(EventType::CAMERA_STOP_PREVIEW, this);

    running_.store(false);
    workerCv_.notify_one();
    if (workerThread_.joinable()) {
        workerThread_.join();
    }

    backend_.shutdown();

    {
        std::lock_guard lock(cameraMutex_);
        requestedState_ = CameraState::Uninitialized;
        updateState(CameraState::Uninitialized);
    }

    return true;
}

std::string_view Camera::name() const {
    return "Camera";
}

bool Camera::canTransition(const CameraState &from, const CameraState &to) {
    switch (from) {
        case CameraState::Uninitialized:
            return to == CameraState::Initializing;

        case CameraState::Initializing:
            return to == CameraState::Idle || to == CameraState::Faulted;

        case CameraState::Idle:
            return to == CameraState::Previewing ||
                   to == CameraState::Recording ||
                   to == CameraState::Faulted ||
                   to == CameraState::ShuttingDown;

        case CameraState::Previewing:
        case CameraState::Recording:
            return to == CameraState::Stopping ||
                   to == CameraState::Faulted ||
                   to == CameraState::ShuttingDown ||
                   to == CameraState::Idle;

        case CameraState::Stopping:
            return to == CameraState::Idle ||
                   to == CameraState::Faulted ||
                   to == CameraState::ShuttingDown;

        case CameraState::Faulted:
            return to == CameraState::Initializing || to == CameraState::ShuttingDown;
        case CameraState::ShuttingDown:
            return to == CameraState::Uninitialized || to == CameraState::Faulted;
    }

    return false;
}

// caller must lock the mutex before a call
void Camera::updateState(const CameraState &to) {
    if (canTransition(currentState_, to)) {
        currentState_ = to;
    } else {
        Logger::error(std::format("Camera: Cannot transition from {} to {}", toString(currentState_), toString(to)));
    }
}

void Camera::workerLoop() {
    while (running_.load()) {
        std::unique_lock lock(cameraMutex_);

        workerCv_.wait(lock, [this]() {
            return !running_.load() ||
                    requestedState_ != currentState_ ||
                    takeSnapShot_;
        });

        if (!running_.load()) {
            return;
        }

        if (takeSnapShot_) {
            takeSnapShot_ = false;
            lock.unlock();
            if (!takeSnapShot()) {
                Logger::error("Camera: could not take snapshot");
            } else {
                Logger::info("Camera: snapshot taken!");
            }
            lock.lock();
        }

        if (requestedState_ == CameraState::Previewing) {
            updateState(CameraState::Previewing);
            lock.unlock();
            const bool previewOk = runPreview();
            lock.lock();
            if (!previewOk) {
                Logger::error("Camera: error while previewing");
                // all the possible falses can spawn from backend issues only
                // they are handled already through setErrorHandler()
                // where error msg is logged and state is updated
            } else {
                requestedState_ == CameraState::ShuttingDown ?
                    updateState(CameraState::ShuttingDown) :
                    updateState(CameraState::Idle);
            }
        }

        if (requestedState_ == CameraState::Recording) {
            updateState(CameraState::Recording);
            requestedState_ = CameraState::Recording;
            // lock.unlock();
            // TODO: call preview, check its returning status, lock mutexes, update states
        }
    }
}

// all these functions must periodically check if they should run
bool Camera::takeSnapShot() {
    // TODO: implement backend handler captureSnapShot()
    return true;
}

bool Camera::runPreview() {
    if (!backend_.configureForPreview()) {
        return false;
    }

    // this actually starts the whole process; we should leave it enough time
    // to work until we call stopCapture() here
    if (!backend_.startCapture()) {
        return false;
    }

    while (running_.load()) {
        {
            std::scoped_lock lock(cameraMutex_);
            if (requestedState_ != CameraState::Previewing) {
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    if (!backend_.stopCapture()) {
        return false;
    }

    return true;
}

bool Camera::runRecording() {
    return true;
}


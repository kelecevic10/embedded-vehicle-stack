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

    updateState(CameraState::Initializing);
    requestedState_ = CameraState::Idle;

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

    updateState(CameraState::Idle);
    return true;
}

bool Camera::stop() {
    if (!running_.load()) {
        Logger::warning("Camera already stopped!");
        return true;
    }

    updateState(CameraState::Stopping);
    cameraMutex_.lock();
    requestedState_ = CameraState::Uninitialized;
    cameraMutex_.unlock();

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

    updateState(CameraState::Uninitialized);

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
                   to == CameraState::Faulted;

        case CameraState::Previewing:
        case CameraState::Recording:
            return to == CameraState::Stopping ||
                   to == CameraState::Faulted;

        case CameraState::Stopping:
            return to == CameraState::Idle ||
                   to == CameraState::Faulted;

        case CameraState::Faulted:
            return to == CameraState::Initializing;
    }

    return false;
}

void Camera::updateState(const CameraState &to) {
    if (canTransition(currentState_, to)) {
        std::scoped_lock lock(cameraMutex_);
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
                updateState(CameraState::Faulted);
                requestedState_ = CameraState::Faulted;
            }

            updateState(CameraState::Idle);
            requestedState_ = CameraState::Idle;
        }

        if (requestedState_ == CameraState::Previewing) {
            lock.unlock();
            if (!startPreview()) {
                updateState(CameraState::Faulted);
                lock.lock();
                requestedState_ = CameraState::Faulted;
            }
            updateState(CameraState::Previewing);
            lock.lock();
            requestedState_ = CameraState::Previewing;
        }

        if (requestedState_ == CameraState::Recording) {
            lock.unlock();
            if (!startRecording()) {
                updateState(CameraState::Faulted);
                lock.lock();
                requestedState_ = CameraState::Faulted;
            }
            updateState(CameraState::Recording);
            lock.lock();
            requestedState_ = CameraState::Recording;
        }
    }
}

bool Camera::takeSnapShot() {

}

bool Camera::startPreview() {
}

bool Camera::startRecording() {
}


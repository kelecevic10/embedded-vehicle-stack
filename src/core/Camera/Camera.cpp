//
// Created by nemanja on 4/23/26.
//

#include "Camera.h"

#include <filesystem>

#include "utils/logger/Logger.h"

static bool saveRawSnapshotToDisk(const FrameData& frame) {
    namespace fs = std::filesystem;

    const fs::path snapshotsDir = "../Images";

    if (!fs::exists(snapshotsDir)) {
        if (!fs::create_directory(snapshotsDir)) {
            Logger::error("Failed to create snapshots directory.");
            return false;
        }
    }

    std::ostringstream baseName;
    baseName << "snapshot_" << frame.sequenceNumber;

    const fs::path metadataPath =
        snapshotsDir / (baseName.str() + "_metadata.txt");

    std::ofstream metadataFile(metadataPath);

    if (!metadataFile.is_open()) {
        Logger::error(std::format(
            "Failed to open metadata file: {}",
            metadataPath.string()
        ));
        return false;
    }

    metadataFile << "sequenceNumber: " << frame.sequenceNumber << '\n';
    metadataFile << "timestampNs: " << frame.timestampNs << '\n';
    metadataFile << "width: " << frame.width << '\n';
    metadataFile << "height: " << frame.height << '\n';
    metadataFile << "pixelFormat: " << frame.pixelFormat << '\n';
    metadataFile << "planes: " << frame.planes.size() << '\n';

    for (std::size_t i = 0; i < frame.planes.size(); ++i) {
        const auto& plane = frame.planes[i];

        const fs::path planePath =
            snapshotsDir / (baseName.str() + "_plane_" + std::to_string(i) + ".raw");

        std::ofstream planeFile(planePath, std::ios::binary);

        if (!planeFile.is_open()) {
            Logger::error(std::format(
                "Failed to open plane file: {}",
                planePath.string()
            ));
            return false;
        }

        planeFile.write(
            reinterpret_cast<const char*>(plane.data.data()),
            static_cast<std::streamsize>(plane.bytesUsed)
        );

        if (!planeFile) {
            Logger::error(std::format(
                "Failed to write plane file: {}",
                planePath.string()
            ));
            return false;
        }

        metadataFile << '\n';
        metadataFile << "plane[" << i << "].file: " << planePath.filename().string() << '\n';
        metadataFile << "plane[" << i << "].bytesUsed: " << plane.bytesUsed << '\n';
        metadataFile << "plane[" << i << "].stride: " << plane.stride << '\n';
        metadataFile << "plane[" << i << "].vectorSize: " << plane.data.size() << '\n';
    }

    Logger::info(std::format(
        "Raw snapshot saved | seq={} | {}x{} | format={} | planes={}",
        frame.sequenceNumber,
        frame.width,
        frame.height,
        frame.pixelFormat,
        frame.planes.size()
    ));

    return true;
}

Camera::Camera(MessageBus* bus)
: Subsystem(), EventProcessor(bus)
{
    currentState_ = CameraState::Uninitialized;
    requestedState_ = CameraState::Uninitialized;
    running_ = false;
    takeSnapShot_ = false;
}

Camera::~Camera() {
    if (running_.load()) {
        Camera::stop();
    }
}

// NOTE: Those handlers will be executed by the listener thread
// to make them easy & lightweight, their job is only to signal the worker thread
// that some job arrived and thread-safely wake them up to work
void Camera::registerHandlers() {
    subscribe(EventType::CAMERA_TAKE_SNAPSHOT, [this](const Event&) {
        std::scoped_lock lock(cameraMutex_);

        if (currentState_ != CameraState::Previewing &&
            requestedState_ != CameraState::Previewing) {
            Logger::warning("Camera: snapshot ignored because preview is not active");
            return;
        }

        takeSnapShot_ = true;
        workerCv_.notify_one();
    });

    subscribe(EventType::CAMERA_START_PREVIEW, [this](const Event&) {
        std::scoped_lock lock(cameraMutex_);
        if (currentState_ == CameraState::Previewing) {
            Logger::warning("Camera already in preview mode!");
            return;
        }
        requestedState_ = CameraState::Previewing;
        workerCv_.notify_one();
    });

    subscribe(EventType::CAMERA_STOP_PREVIEW, [this](const Event&) {
        std::scoped_lock lock(cameraMutex_);
        if (currentState_ != CameraState::Previewing) {
            Logger::warning("Camera not in preview mode!");
            return;
        }
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

    backend_.setPreviewFrameCallback([](const FrameView& frame) {
        if (frame.sequenceNumber % 30 != 0) {
            return;
        }

        bool valid = true;
        for (const auto& plane : frame.planes) {
            if (plane.data == nullptr) {
                valid = false;
                break;
            }
        }

        Logger::info(std::format(
            "Camera PREVIEW frame received: {} | seq={} | ts={}ns | {}x{} | format={} | planes={}",
            valid ? "VALID" : "INVALID",
            frame.sequenceNumber,
            frame.timestampNs,
            frame.width,
            frame.height,
            frame.pixelFormat,
            frame.planes.size()
        ));
    });

    backend_.setSnapshotFrameCallback([](const FrameData&& frame) {
       Logger::info(std::format(
            "Camera SNAPSHOT frame received | seq={} | ts={}ns | {}x{} | format={} | planes={}",
            frame.sequenceNumber,
            frame.timestampNs,
            frame.width,
            frame.height,
            frame.pixelFormat,
            frame.planes.size()
        ));

        if (!saveRawSnapshotToDisk(frame)) {
            Logger::error("Failed to save raw snapshot to disk.");
        }
    });

    backend_.setErrorCallback([this](const std::string& error) {
        Logger::error(std::format("Camera error: {}", error));
        std::scoped_lock lock(cameraMutex_);
        updateState(CameraState::Faulted);
        requestedState_ = CameraState::Faulted;
    });

    backend_.setWarningCallback([](const std::string& warning) {
       Logger::warning(std::format("Camera warning: {}", warning));
    });

    if (!backend_.initialize()) {
        std::scoped_lock lock(cameraMutex_);
        updateState(CameraState::Faulted);
        requestedState_ = CameraState::Faulted;
        Logger::error("Camera: could not initialize backend!");
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
        Event stateChanged {
            EventType::SUBSYSTEM_STATE_CHANGED,
            std::format("{}: {} -> {}", name(), toString(currentState_), toString(to)),
        };
        publish(std::move(stateChanged));
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

        if (currentState_ == CameraState::Idle && requestedState_ == CameraState::Previewing)
        {
            updateState(CameraState::Previewing);

            lock.unlock();
            const bool previewStarted = startPreviewMode();
            lock.lock();

            if (!previewStarted) {
                updateState(CameraState::Faulted);
                requestedState_ = CameraState::Faulted;
            }
            continue;
        }

        if (currentState_ == CameraState::Previewing && takeSnapShot_)
        {
            takeSnapShot_ = false;

            lock.unlock();
            const bool snapshotTaken = takeSnapShot();
            lock.lock();
            if (!snapshotTaken) {
                Logger::error("Camera: could not request snapshot");
            } else {
                Logger::info("Camera: snapshot requested");
            }
            continue;
        }

        if (currentState_ == CameraState::Previewing && requestedState_ == CameraState::Idle)
        {
            updateState(CameraState::Stopping);

            lock.unlock();
            const bool previewStopped = stopPreviewMode();
            lock.lock();

            if (!previewStopped) {
                updateState(CameraState::Faulted);
                requestedState_ = CameraState::Faulted;
            } else {
                updateState(CameraState::Idle);
            }
            continue;
        }

        if (requestedState_ == CameraState::ShuttingDown) {
            if (currentState_ == CameraState::Previewing) {
                lock.unlock();
                const bool previewStopped = stopPreviewMode();
                lock.lock();

                if (!previewStopped) {
                    updateState(CameraState::Faulted);
                    requestedState_ = CameraState::Faulted;
                    continue;
                }
            }

            updateState(CameraState::ShuttingDown);
        }
    }
}



bool Camera::takeSnapShot() {
    if (!backend_.requestSnapshot()) {
        Logger::error("Camera: could not request snapshot");
        return false;
    }

    return true;
}

bool Camera::startPreviewMode() {
    if (!backend_.configureForPreviewAndSnapshots()) {
        Logger::error("Camera: could not configure for preview + snapshot");
        return false;
    }

    if (!backend_.startPreview()) {
        Logger::error("Camera: could not start preview");
        return false;
    }

    return true;
}

bool Camera::stopPreviewMode() {
    if (!backend_.stopPreview()) {
        Logger::error("Camera: could not stop preview");
        return false;
    }

    return true;
}


bool Camera::runRecording() {
    return true;
}


//
// Created by nemanja on 4/25/26.
//

#include "CameraBackend.h"

#include "utils/logger/Logger.h"

CameraBackend::~CameraBackend() {
    shutdown();
}

bool CameraBackend::initialize() {
    if (initialized_) {
        reportError("CameraBackend is already initialized");
        return false;
    }

    cameraManager_ = std::make_unique<CameraManager>();
    const int startRes = cameraManager_->start();
    if (startRes != 0) {
        reportError("Failed to start CameraManager");
        return false;
    }

    const auto& cameras = cameraManager_->cameras();
    if (cameras.empty()) {
        reportError("No cameras found by libcamera.");
        cameraManager_->stop();
        cameraManager_.reset();
        return false;
    }

    camera_ = cameras.front();
    if (!camera_) {
        reportError("Failed to get camera instance.");
        cameraManager_->stop();
        cameraManager_.reset();
        return false;
    }

    const int acquireRes = camera_->acquire();
    if (acquireRes != 0) {
        reportError("Failed to acquire camera.");
        camera_.reset();
        cameraManager_->stop();
        cameraManager_.reset();
        return false;
    }

    initialized_ = true;
    return true;
}

void CameraBackend::shutdown() {
    if (!initialized_) {
        return;
    }

    if (capturing_) {
        stopCapture();
    }

    activeConfiguration_.reset();

    if (camera_) {
        camera_->release();
        camera_.reset();
    }

    if (cameraManager_) {
        cameraManager_->stop();
        cameraManager_.reset();
    }

    initialized_ = false;
}

void CameraBackend::reportError(const std::string &error) const {
    if (errorCallback_) {
        errorCallback_(error);
    }
}

void CameraBackend::setFrameCallback(FrameCallback callback) {
    frameCallback_ = std::move(callback);
}

void CameraBackend::setErrorCallback(ErrorCallback callback) {
    errorCallback_ = std::move(callback);
}

bool CameraBackend::configureFromSpec(const CameraConfigSpec& spec) {
    if (!initialized_) {
        reportError("Cannot configure camera: backend is not initialized!");
        return false;
    }

    auto configuration = camera_->generateConfiguration(
        {   spec.role   }
        );

    if (!configuration) {
        reportError("Failed to generate configuration!");
        return false;
    }

    const libcamera::CameraConfiguration::Status validationStatus = configuration->validate();
    if (validationStatus == libcamera::CameraConfiguration::Invalid) {
        reportError("Camera configuration is invalid");
        return false;
    }

    const int configureResult = camera_->configure(configuration.get());
    if (configureResult != 0) {
        reportError("Failed to apply configuration!");
        return false;
    }

    activeConfiguration_ = std::move(configuration);
    return true;
}

bool CameraBackend::configureForPreview() {
    return configureFromSpec(CameraConfigSpec {
        .role = libcamera::StreamRole::Viewfinder
    });
}

bool CameraBackend::configureForRecording() {
    return configureFromSpec(CameraConfigSpec {
        .role = libcamera::StreamRole::VideoRecording
    });
}

bool CameraBackend::configureForSnapshot() {
    return configureFromSpec(CameraConfigSpec {
        .role = libcamera::StreamRole::StillCapture
    });
}

bool CameraBackend::startCapture() {
    if (!initialized_) {
        reportError("Cannot start capture: backend is not initialized!");
        return false;
    }

    if (!activeConfiguration_) {
        reportError("Cannot start capture: no configuration active!");
        return false;
    }

    if (capturing_) {
        reportError("Capture already in progress!");
        return false;
    }

    if (activeConfiguration_->empty()) {
        reportError("Cannot start capture: active configuration has no streams!");
        return false;
    }

    activeStream_ = activeConfiguration_->at(0).stream();
    if (!activeStream_) {
        reportError("Failed to get active stream!");
        return false;
    }

    bufferAllocator_ = std::make_unique<FrameBufferAllocator>(camera_);
    const int allocateResult = bufferAllocator_->allocate(activeStream_);
    if (allocateResult < 0) {
        reportError("Failed to allocate frame buffers for active stream");
        bufferAllocator_.reset();
        activeStream_ = nullptr;
        return false;
    }

    const auto& buffers = bufferAllocator_->buffers(activeStream_);
    if (buffers.empty()) {
        reportError("No frame buffers were allocated for active stream.");
        bufferAllocator_.reset();
        activeStream_ = nullptr;
        return false;
    }

    requests_.clear();
    requests_.reserve(buffers.size());

    for (auto& buffer : buffers) {
        std::unique_ptr<Request> request = camera_->createRequest();
        if (!request) {
            reportError("Failed to create capture request!");
            requests_.clear();
            bufferAllocator_.reset();
            activeStream_ = nullptr;
            return false;
        }
        const int addBufferResult = request->addBuffer(activeStream_, buffer.get());
        if (addBufferResult < 0) {
            reportError("Failed to attach buffer to capture request!");
            requests_.clear();
            bufferAllocator_.reset();
            activeStream_ = nullptr;
            return false;
        }

        requests_.push_back(std::move(request));
    }

    camera_->requestCompleted.connect(this, &CameraBackend::onRequestCompleted);
    const int startResult = camera_->start();
    if (startResult < 0) {
        reportError("Failed to start camera capture pipeline!");
        camera_->requestCompleted.disconnect(this, &CameraBackend::onRequestCompleted);
        requests_.clear();
        bufferAllocator_.reset();
        activeStream_ = nullptr;
        return false;
    }

    capturing_ = true;

    for (auto& request : requests_) {
        const int queueResult = camera_->queueRequest(request.get());
        if (queueResult < 0) {
            reportError("Failed to queue initial capture request!");
            capturing_ = false;
            camera_->stop();
            camera_->requestCompleted.disconnect(this, &CameraBackend::onRequestCompleted);
            requests_.clear();
            bufferAllocator_.reset();
            activeStream_ = nullptr;
            return false;
        }
    }


    return true;
}

bool CameraBackend::stopCapture() {
    if (!capturing_) {
        return true;
    }

    capturing_ = false;

    if (!camera_) {
        reportError("Cannot stop capture: camera instance is null.");
        requests_.clear();
        bufferAllocator_.reset();
        activeStream_ = nullptr;
        return false;
    }

    const int stopResult = camera_->stop();
    if (stopResult < 0) {
        reportError("Failed to stop camera capture pipeline!");
        return false;
    }

    camera_->requestCompleted.disconnect(this, &CameraBackend::onRequestCompleted);
    requests_.clear();
    bufferAllocator_.reset();
    activeStream_ = nullptr;

    return true;
}

void CameraBackend::onRequestCompleted(Request *request) {
    if (!request) {
        reportError("Received null completed request.");
        return;
    }

    if (request->status() == Request::RequestCancelled) {
        return;
    }

    if (!capturing_) {
        return;
    }

    if (!activeStream_) {
        reportError("Active stream is null in request completion handler.");
        return;
    }

    const auto it = request->buffers().find(activeStream_);
    if (it == request->buffers().end() || !it->second) {
        reportError("Completed request does not contain a valid buffer for the active stream.");
        return;
    }

    if (!activeConfiguration_ || activeConfiguration_->empty()) {
        reportError("Active configuration is missing in request completion handler");
        return;
    }

    FrameBuffer* buffer = it->second;
    FrameData frame {};
    frame.sequenceNumber = request->sequence();
    frame.timestampNs = buffer->metadata().timestamp;
    frame.width = activeConfiguration_->at(0).size.width;
    frame.height = activeConfiguration_->at(0).size.height;
    frame.pixelFormat = activeConfiguration_->at(0).pixelFormat.toString();

    frame.planes.reserve(buffer->planes().size());
    for (size_t i = 0; i < buffer->planes().size(); i++) {
        const auto& plane = buffer->planes()[i];
        const auto& metaPlane = buffer->metadata().planes()[i];

        FramePlaneView planeView {};
        planeView.data = nullptr;           // TODO: map plane memory properly
        planeView.length = plane.length;
        planeView.bytesUsed = metaPlane.bytesused;
        planeView.stride = 0;               // TODO: populate when plane-row layout is know

        frame.planes.push_back(planeView);
    }

    if (frameCallback_) {
        frameCallback_(frame);
    }

    request->reuse(Request::ReuseBuffers);
    const int queueResult = camera_->queueRequest(request);
    if (queueResult < 0) {
        reportError("Failed to re-queue capture request!");
        capturing_ = false;
    }
}

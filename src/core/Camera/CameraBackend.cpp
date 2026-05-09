//
// Created by nemanja on 4/25/26.
//

#include "CameraBackend.h"
#include "utils/logger/Logger.h"
#include <sys/mman.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>


CameraBackend::~CameraBackend() {
    shutdown();
}

bool CameraBackend::initialize() {
    if (initialized_) {
        reportWarning("CameraBackend is already initialized");
        return true;
    }

    cameraManager_ = std::make_unique<CameraManager>();
    const int startRes = cameraManager_->start();
    if (startRes != 0) {
        cameraManager_.reset();
        reportError("Failed to start CameraManager");
        return false;
    }

    const auto& cameras = cameraManager_->cameras();
    if (cameras.empty()) {
        reportError("No cameras found by backend.");
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

    if (previewRunning_) {
        stopPreview();
    } else {
        // Even if preview is not running, partially allocated capture resources may exist
        // after a failed start/configuration attempt, so clean them explicitly
        clearCaptureResources();
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

void CameraBackend::reportWarning(const std::string &warning) const {
    if (warningCallback_) {
        warningCallback_(warning);
    }
}

void CameraBackend::setPreviewFrameCallback(PreviewFrameCallback callback) {
    previewFrameCallback_ = std::move(callback);
}

void CameraBackend::setSnapshotFrameCallback(SnapshotFrameCallback callback) {
    snapshotFrameCallback_ = std::move(callback);
}

void CameraBackend::setErrorCallback(ErrorCallback callback) {
    errorCallback_ = std::move(callback);
}

void CameraBackend::setWarningCallback(WarningCallback callback) {
    warningCallback_ = std::move(callback);
}

// This function assumes that libcamera preserves the order of requested
// StreamRole values when generating CameraConfiguration entries.
// That lets us map configuration[i].stream() back to spec.roles[i]
// and assign it to previewStream_, snapshotStream_ or recordingStream_.
bool CameraBackend::configureFromSpec(const CameraConfigSpec& spec) {
    if (!initialized_) {
        reportError("Cannot configure camera: backend is not initialized!");
        return false;
    }

    if (previewRunning_) {
        reportError("Cannot configure camera: preview is running!");
        return false;
    }

    if (spec.roles.empty()) {
        reportError("Cannot configure camera: no roles specified!");
        return false;
    }

    auto configuration = camera_->generateConfiguration(spec.roles);
    if (!configuration) {
        reportError("Failed to generate configuration!");
        return false;
    }

    if (configuration->empty()) {
        reportError("Generated configuration has no streams!");
        return false;
    }

    const CameraConfiguration::Status validationStatus = configuration->validate();
    if (validationStatus == CameraConfiguration::Invalid) {
        reportError("Camera configuration is invalid");
        return false;
    }
    if (validationStatus == CameraConfiguration::Adjusted) {
        reportWarning("Camera configuration was adjusted by libcamera");
    }

    const int configurationRes = camera_->configure(configuration.get());
    if (configurationRes != 0) {
        reportError("Failed to apply camera configuration!");
        previewStream_ = nullptr;
        recordingStream_ = nullptr;
        snapshotStream_ = nullptr;
        return false;
    }

    previewStream_ = nullptr;
    recordingStream_ = nullptr;
    snapshotStream_ = nullptr;

    for (std::size_t i = 0; i < spec.roles.size(); i++) {
        if (i >= configuration->size()) {
            reportError("Generated configuration has less streams than requested!");
            return false;
        }

        Stream* stream = configuration->at(i).stream();
        if (!stream) {
            reportError("Generated configuration contains null stream!");
            return false;
        }

        switch (spec.roles[i]) {
            case StreamRole::Viewfinder:
                previewStream_ = stream;
                break;
            case StreamRole::VideoRecording:
                recordingStream_ = stream;
                break;
            case StreamRole::StillCapture:
                snapshotStream_ = stream;
                break;

            default:
                reportWarning("Unsupported stream role found while configuring camera.");
                break;
        }
    }

    if (std::ranges::find(spec.roles, StreamRole::Viewfinder) != spec.roles.end() && !previewStream_) {
        reportError("Preview stream was requested but not configured.");
        return false;
    }

    if (std::ranges::find(spec.roles, StreamRole::VideoRecording) != spec.roles.end() && !recordingStream_) {
        reportError("Recording stream was requested but not configured");
        return false;
    }

    if (std::ranges::find(spec.roles, StreamRole::StillCapture) != spec.roles.end() && !snapshotStream_) {
        reportError("Snapshot stream was requested but not configured");
        return false;
    }

    activeConfiguration_ = std::move(configuration);
    return true;
}

bool CameraBackend::configureForPreviewOnly() {
    return configureFromSpec(CameraConfigSpec {
        .roles = {
            StreamRole::Viewfinder
        }
    });
}

bool CameraBackend::configureForPreviewAndSnapshots() {
    return configureFromSpec(CameraConfigSpec {
        .roles = {
            StreamRole::Viewfinder,
            StreamRole::StillCapture
        }
    });
}

// bool CameraBackend::configureForSnapshot() {
//     return configureFromSpec(CameraConfigSpec {
//         .role = libcamera::StreamRole::StillCapture
//     });
// }

bool CameraBackend::createPreviewRequests() {
    if (!camera_) {
        reportError("Cannot create preview requests: camera instance is null.");
        return false;
    }

    if (!previewStream_) {
        reportError("Cannot create preview requests: preview stream is null.");
        return false;
    }

    if (!bufferAllocator_) {
        reportError("Cannot create preview requests: buffer allocator is null.");
        return false;
    }

    const int allocateRes = bufferAllocator_->allocate(previewStream_);
    if (allocateRes < 0) {
        reportError("Cannot create preview requests: failed to allocate buffers for preview stream.");
        previewRequests_.clear();
        return false;
    }

    if (!mapBuffersForStream(previewStream_)) {
        reportError("Cannot create preview requests: failed to map buffers for preview stream.");
        previewRequests_.clear();
        return false;
    }

    const auto& previewBuffers = bufferAllocator_->buffers(previewStream_);
    if (previewBuffers.empty()) {
        reportError("Cannot create preview requests: no buffers allocated for preview stream.");
        previewRequests_.clear();
        return false;
    }

    previewRequests_.clear();
    previewRequests_.reserve(previewBuffers.size());

    for (auto& buffer : previewBuffers) {
        std::unique_ptr<Request> request = camera_->createRequest();
        if (!request) {
            reportError("Cannot create preview requests: failed to create request.");
            previewRequests_.clear();
            return false;
        }

        const int addBufferRes = request->addBuffer(previewStream_, buffer.get());
        if (addBufferRes < 0) {
            reportError("Cannot create preview requests: failed to attach buffer to request.");
            previewRequests_.clear();
            return false;
        }

        previewRequests_.push_back(std::move(request));
    }

    return true;
}

bool CameraBackend::createSnapshotRequest() {
    if (!camera_) {
        reportError("Cannot create snapshot request: camera instance is null.");
        return false;
    }

    if (!snapshotStream_) {
        reportError("Cannot create snapshot request: snapshot stream is null.");
        return false;
    }

    if (!bufferAllocator_) {
        reportError("Cannot create snapshot request: buffer allocator is null.");
        return false;
    }

    const int allocateRes = bufferAllocator_->allocate(snapshotStream_);
    if (allocateRes < 0) {
        reportError("Cannot create snapshot request: failed to allocate buffers for snapshot stream.");
        snapshotRequest_.reset();
        return false;
    }

    if (!mapBuffersForStream(snapshotStream_)) {
        reportError("Cannot create snapshot request: failed to map buffers for snapshot stream.");
        snapshotRequest_.reset();
        return false;
    }

    const auto& snapshotBuffers = bufferAllocator_->buffers(snapshotStream_);
    if (snapshotBuffers.empty()) {
        reportError("Cannot create snapshot request: no buffers allocated for snapshot stream.");
        snapshotRequest_.reset();
        return false;
    }

    snapshotRequest_ = camera_->createRequest();
    if (!snapshotRequest_) {
        reportError("Cannot create snapshot request: failed to create request.");
        return false;
    }

    const int addBufferRes =
            snapshotRequest_->addBuffer(snapshotStream_, snapshotBuffers.front().get());
    if (addBufferRes < 0) {
        reportError("Cannot create snapshot request: failed to attach buffer to request.");
        snapshotRequest_.reset();
        return false;
    }

    return true;
}


bool CameraBackend::startPreview() {
    if (!initialized_) {
        reportError("Cannot start preview: backend is not initialized!");
        return false;
    }

    if (!camera_) {
        reportError("Cannot start preview: camera instance is null!");
        return false;
    }

    if (!activeConfiguration_) {
        reportError("Cannot start preview: no active configuration found!");
        return false;
    }

    if (previewRunning_) {
        reportWarning("startPreview() called while preview is already running.");
        return true;
    }

    if (activeConfiguration_->empty()) {
        reportError("Cannot start preview: active configuration has no streams!");
        return false;
    }

    if (!previewStream_) {
        reportError("Cannot start preview: preview stream is not configured!");
        return false;
    }

    bufferAllocator_ = std::make_unique<FrameBufferAllocator>(camera_);

    if (!createPreviewRequests()) {
        reportError("Failed to create preview requests!");
        clearCaptureResources();
        return false;
    }

    if (snapshotStream_) {
        if (!createSnapshotRequest()) {
            reportError("Failed to create snapshot request!");
            clearCaptureResources();
            return false;
        }
    }

    camera_->requestCompleted.connect(this, &CameraBackend::onRequestCompleted);

    const int startResult = camera_->start();
    if (startResult < 0) {
        reportError("Failed to start camera preview pipeline!");
        camera_->requestCompleted.disconnect(this, &CameraBackend::onRequestCompleted);
        clearCaptureResources();
        return false;
    }

    previewRunning_ = true;

    // Snapshot request is not queued now. It is only prepared, so later,
    // when Camera calls backend_.requestSnapshot(), we queue it separately.
    for (auto& request : previewRequests_) {
        const int queueResult = camera_->queueRequest(request.get());

        if (queueResult < 0) {
            reportError("Failed to queue preview request!");
            previewRunning_ = false;
            camera_->stop();
            camera_->requestCompleted.disconnect(this, &CameraBackend::onRequestCompleted);
            clearCaptureResources();
            return false;
        }
    }

    return true;
}

bool CameraBackend::stopPreview() {
    if (!previewRunning_) {
        reportWarning("stopPreview() called while preview is not running.");
        return true;
    }

    previewRunning_ = false;
    snapshotPending_ = false;

    if (!camera_) {
        reportError("Cannot stop preview: camera instance is null.");
        clearCaptureResources();
        return false;
    }

    const int stopResult = camera_->stop();
    if (stopResult < 0) {
        reportError("Failed to stop camera preview pipeline.");
        return false;
    }

    camera_->requestCompleted.disconnect(this, &CameraBackend::onRequestCompleted);

    clearCaptureResources();

    return true;
}

bool CameraBackend::requestSnapshot() {
    if (!previewRunning_) {
        reportError("Cannot request snapshot: preview is not running.");
        return false;
    }

    if (!snapshotStream_) {
        reportError("Cannot request snapshot: snapshot stream is not configured.");
        return false;
    }

    if (!camera_) {
        reportError("Cannot request snapshot: camera instance is null.");
        return false;
    }

    if (!snapshotRequest_) {
        reportError("Cannot request snapshot: snapshot request is not prepared.");
        return false;
    }

    if (snapshotPending_) {
        reportWarning("Snapshot request ignored: snapshot is already pending.");
        return true;
    }

    snapshotPending_ = true;

    const int queueRes = camera_->queueRequest(snapshotRequest_.get());
    if (queueRes < 0) {
        reportWarning("Failed to queue snapshot request.");
        snapshotPending_ = false;
        return false;
    }

    return true;
}

bool CameraBackend::buildFrameView(Request *request, Stream *stream, FrameType type, FrameView &outFrameView) {
    if (!request) {
        reportError("Cannot build frame view: received null request.");
        return false;
    }

    if (!stream) {
        reportError("Cannot build frame view: received null stream.");
        return false;
    }

    const auto it = request->buffers().find(stream);
    if (it == request->buffers().end() || !it->second) {
        reportError("Cannot build frame view: request does not contain a valid buffer for the stream.");
        return false;
    }

    FrameBuffer* buffer = it->second;

    const auto& streamConfig = stream->configuration();

    const auto& planes = buffer->planes();
    const auto& metaPlanes = buffer->metadata().planes();
    if (metaPlanes.size() < planes.size()) {
        reportError("Cannot build frame view: frame metadata contains fewer planes than buffer.");
        return false;
    }

    const auto mappedBufferIt = mappedBuffers_.find(buffer);
    if (mappedBufferIt == mappedBuffers_.end()) {
        reportError("Cannot build frame view: buffer is not mapped.");
        return false;
    }
    const MappedBuffer& mappedBuffer = mappedBufferIt->second;
    if (mappedBuffer.planes.size() < planes.size()) {
        reportError("Cannot build frame view: mapped buffer contains fewer planes than frame buffer");
        return false;
    }

    outFrameView = FrameView {};
    outFrameView.type = type;
    outFrameView.sequenceNumber = request->sequence();
    outFrameView.timestampNs = buffer->metadata().timestamp;
    outFrameView.width = streamConfig.size.width;
    outFrameView.height = streamConfig.size.height;
    outFrameView.pixelFormat = streamConfig.pixelFormat.toString();
    outFrameView.planes.reserve(planes.size());

    for (std::size_t i = 0; i < planes.size(); i++) {

        const auto& metaPlane = metaPlanes[i];
        const auto& mappedPlane = mappedBuffer.planes[i];

        if (!mappedPlane.data) {
            reportError("Cannot build frame view: mapped plane data is null.");
            return false;
        }

        if (metaPlane.bytesused > mappedPlane.length) {
            reportError("Cannot build frame view: bytesUsed exceeds mapped plane length.");
            return false;
        }

        FramePlaneView planeView {};
        planeView.data = mappedPlane.data;
        planeView.length = mappedPlane.length;
        planeView.bytesUsed = metaPlane.bytesused;
        planeView.stride = streamConfig.stride;

        outFrameView.planes.push_back(planeView);
    }

    return true;
}

bool CameraBackend::makeOwnedFrameData(const FrameView &frameView, FrameData &frameData) {
    frameData = FrameData {};
    frameData.type = frameView.type;
    frameData.sequenceNumber = frameView.sequenceNumber;
    frameData.timestampNs = frameView.timestampNs;
    frameData.width = frameView.width;
    frameData.height = frameView.height;
    frameData.pixelFormat = frameView.pixelFormat;

    frameData.planes.reserve(frameView.planes.size());
    for (const auto& planeView : frameView.planes) {
        FramePlaneData planeData {};

        if (planeView.data == nullptr) {
            reportError("Cannot make owned frame data: plane data is null.");
            return false;
        }

        if (planeView.bytesUsed == 0) {
            reportError("Cannot make owned frame data: plane bytesUsed is 0.");
            return false;
        }

        if (planeView.bytesUsed > planeView.length) {
            reportError("Cannot make owned frame data: plane bytesUsed exceeds plane length.");
            return false;
        }

        // copy metadata
        planeData.bytesUsed = planeView.bytesUsed;
        planeData.stride = planeView.stride;

        // copy data
        planeData.data.resize(planeView.bytesUsed);
        std::memcpy(
            planeData.data.data(),
            planeView.data,
            planeView.bytesUsed
        );
        
        frameData.planes.push_back(std::move(planeData));
    }

    return true;
}

void CameraBackend::onRequestCompleted(Request* request) {
    if (!request) {
        reportError("Received null completed request.");
        return;
    }

    if (request->status() == Request::RequestCancelled) {
        return;
    }

    if (!previewRunning_) {
        return;
    }

    if (!camera_) {
        reportError("Cannot handle completed request: camera instance is null.");
        return;
    }

    const auto& buffers = request->buffers();

    const bool hasPreviewBuffer =
        previewStream_ && buffers.contains(previewStream_);

    const bool hasSnapshotBuffer =
        snapshotStream_ && buffers.contains(snapshotStream_);

    if (!hasPreviewBuffer && !hasSnapshotBuffer) {
        reportWarning("Completed request does not contain any known stream buffer.");
        return;
    }

    if (hasPreviewBuffer) {
        FrameView previewFrame {};
        if (buildFrameView(request, previewStream_, FrameType::Preview, previewFrame)) {
            if (previewFrameCallback_) {
                previewFrameCallback_(previewFrame);
            }
        }
    }

    if (hasSnapshotBuffer) {
        FrameView snapshotFrame {};

        if (buildFrameView(request, snapshotStream_, FrameType::Snapshot, snapshotFrame)) {
            FrameData frameData {};
            if (makeOwnedFrameData(snapshotFrame, frameData)) {
                if (snapshotFrameCallback_) {
                    snapshotFrameCallback_(std::move(frameData));
                }
            } else {
                reportWarning("Failed to create frame data from snapshot frame.");
            }
        }

        snapshotPending_ = false;

        request->reuse(Request::ReuseBuffers);
        // Snapshot requests are one-shot requests.
        // They are not reused/requeued automatically.
        return;
    }

    request->reuse(Request::ReuseBuffers);

    const int queueResult = camera_->queueRequest(request);

    if (queueResult < 0) {
        reportError("Failed to re-queue preview request.");
        return;
    }
}



// Must be called only after the camera pipeline has been stopped,
// or after a failed startup path where requests were never queued.
// Preview requests and frame buffers must not be destroyed while
// libcamera may still own or process them.
void CameraBackend::clearCaptureResources() {
    previewRequests_.clear();
    recordingRequests_.clear();
    snapshotRequest_.reset();

    unmapFrameBuffers();

    bufferAllocator_.reset();

    previewStream_ = nullptr;
    snapshotStream_ = nullptr;
    recordingStream_ = nullptr;

    snapshotPending_ = false;
    previewRunning_ = false;
}

bool CameraBackend::mapFrameBuffer(FrameBuffer *buffer) {
    if (!buffer) {
        reportError("Cannot map frame buffer: received null buffer.");
        return false;
    }

    if (mappedBuffers_.contains(buffer)) {
        return true;
    }

    const long pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize <= 0) {
        reportError("Cannot map frame buffer: failed to get system page size.");
        return false;
    }

    MappedBuffer mappedBuffer {};
    const auto& planes = buffer->planes();

    mappedBuffer.planes.reserve(planes.size());

    for (const auto& plane: planes) {

        const off_t planeOffset = static_cast<off_t>(plane.offset);

        // mmap() requires the offset to be aligned to the system page size
        // if the actual plane starts at a non-aligned offset, we map from the
        // previous aligned offset and then shift the data pointer manually.
        const off_t alignedOffset = (planeOffset / pageSize) * pageSize;

        const size_t offsetDelta = static_cast<size_t>(planeOffset - alignedOffset);

        const size_t mappedLength = plane.length + offsetDelta;

        void* mappedAddress = mmap(
            nullptr,
            mappedLength,
            PROT_READ,
            MAP_SHARED,
            plane.fd.get(),
            alignedOffset
        );

        if (mappedAddress == MAP_FAILED) {
            reportError(std::format(
                "Failed to map frame buffer: {}.",
                strerror(errno)
                ));

            // Roll back all planes that were already mapped for this buffer
            for (const auto& mappedPlane: mappedBuffer.planes) {
                if (mappedPlane.mappedAddress &&
                    mappedPlane.mappedAddress != MAP_FAILED &&
                    mappedPlane.mappedLength > 0) {
                    munmap(mappedPlane.mappedAddress, mappedPlane.mappedLength);
                }
            }
            return false;
        }

        const auto* data = reinterpret_cast<const std::byte*> (
            static_cast<const char*>(mappedAddress) + offsetDelta
        );

        mappedBuffer.planes.push_back(MappedPlane {
            .mappedAddress = mappedAddress,
            .data = data,
            .mappedLength = mappedLength,
            .length = plane.length
        });
    }

    mappedBuffers_.emplace(buffer, std::move(mappedBuffer));
    return true;
}

bool CameraBackend::mapBuffersForStream(Stream *stream) {
    if (!stream) {
        reportError("Cannot map buffers for stream: received null stream.");
        return false;
    }

    if (!bufferAllocator_) {
        reportError("Cannot map buffers for stream: buffer allocator is null.");
        return false;
    }

    const auto& buffers = bufferAllocator_->buffers(stream);
    if (buffers.empty()) {
        reportError("Cannot map buffers for stream: no buffers allocated for stream.");
        return false;
    }

    for (const auto& buffer: buffers) {
        if (!mapFrameBuffer(buffer.get())) {
            return false;
        }
    }

    return true;
}

void CameraBackend::unmapFrameBuffers() {
    for (auto& [buffer, mappedBuffer]: mappedBuffers_) {
        for (auto& plane : mappedBuffer.planes) {
            if (plane.mappedAddress &&
                plane.mappedAddress != MAP_FAILED &&
                plane.mappedLength > 0) {
                munmap(plane.mappedAddress, plane.mappedLength);
                }

            plane.mappedAddress = nullptr;
            plane.data = nullptr;
            plane.mappedLength = 0;
            plane.length = 0;
        }
    }

    mappedBuffers_.clear();
}

//
// Created by nemanja on 4/25/26.
//

#ifndef EMBEDDED_VEHICLE_STACK_CAMERABACKEND_H
#define EMBEDDED_VEHICLE_STACK_CAMERABACKEND_H
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <libcamera/libcamera.h>
#include <cstddef>
#include <vector>
#include <libcamera/control_ids.h>
#include <optional>

using CameraManager = libcamera::CameraManager;
using Stream = libcamera::Stream;
using FrameBufferAllocator = libcamera::FrameBufferAllocator;
using Request = libcamera::Request;
using FrameBuffer = libcamera::FrameBuffer;
using CameraConfiguration = libcamera::CameraConfiguration;
using StreamRole = libcamera::StreamRole;

struct FramePlaneView {
    const std::byte* data;   // Pointer to the beginning of this plane's memory region.
    size_t length;           // Total size of this plane's allocated memory region, in bytes.
    size_t bytesUsed;        // Number of valid bytes currently containing frame data.
    size_t stride;           // How many bytes you move in memory to go from one plane row to the next.
};

enum class FrameType {
    Preview,
    Snapshot,
    Recording
};

struct FrameData {
    FrameType type;
    uint64_t sequenceNumber;                // Number identifying this frame in the capture sequence.
    uint64_t timestampNs;                   // Timestamp expressed in nanoseconds
    uint32_t width;
    uint32_t height;
    std::string pixelFormat;                // RGB / YUW
    std::vector<FramePlaneView> planes;     // Memory views of all planes that make up this frame
};

struct CameraConfigSpec {
    std::vector<StreamRole> roles;

    std::optional<uint32_t> previewWidth;
    std::optional<uint32_t> previewHeight;
    std::optional<double> previewFps;

    std::optional<uint32_t> snapshotWidth;
    std::optional<uint32_t> snapshotHeight;
};


class CameraBackend {
public:
    using PreviewFrameCallback = std::function<void(const FrameData&)>;
    using SnapshotFrameCallback = std::function<void(const FrameData&)>;
    using ErrorCallback = std::function<void(const std::string&)>;
    using WarningCallback = std::function<void(const std::string&)>;

    CameraBackend() = default;
    ~CameraBackend();

    bool initialize();
    void shutdown();

    bool configureForPreviewOnly();
    bool configureForPreviewAndSnapshots();

    bool startPreview();
    bool stopPreview();
    bool requestSnapshot();

    void reportError(const std::string& error) const;
    void reportWarning(const std::string& warning) const;

    void setPreviewFrameCallback(PreviewFrameCallback callback);
    void setSnapshotFrameCallback(SnapshotFrameCallback callback);
    void setErrorCallback(ErrorCallback callback);
    void setWarningCallback(WarningCallback callback);

private:
    std::unique_ptr<CameraManager> cameraManager_ { nullptr };
    std::shared_ptr<libcamera::Camera> camera_ { nullptr };
    std::unique_ptr<CameraConfiguration> activeConfiguration_ { nullptr };

    PreviewFrameCallback previewFrameCallback_;
    SnapshotFrameCallback snapshotFrameCallback_;
    ErrorCallback errorCallback_;
    WarningCallback warningCallback_;

    Stream* previewStream_ { nullptr };
    Stream* recordingStream_ { nullptr };
    Stream* snapshotStream_ { nullptr };

    std::vector<std::unique_ptr<Request>> previewRequests_;
    std::vector<std::unique_ptr<Request>> recordingRequests_;
    std::unique_ptr<Request> snapshotRequest_;

    bool initialized_ { false };
    bool previewRunning_ {false};
    bool snapshotPending_ {false};
    std::unique_ptr<FrameBufferAllocator> bufferAllocator_;

    // methods
    bool configureFromSpec(const CameraConfigSpec& spec);
    void onRequestCompleted(Request* request);

    bool createPreviewRequests();
    bool createSnapshotRequest();

    bool buildFrameData(Request* request, Stream* stream, FrameType type, FrameData& outFrame);
    void clearCaptureResources();
};


#endif //EMBEDDED_VEHICLE_STACK_CAMERABACKEND_H

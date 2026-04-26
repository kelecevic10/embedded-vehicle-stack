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

using CameraManager = libcamera::CameraManager;
using Stream = libcamera::Stream;
using FrameBufferAllocator = libcamera::FrameBufferAllocator;
using Request = libcamera::Request;
using FrameBuffer = libcamera::FrameBuffer;
using CameraConfiguration = libcamera::CameraConfiguration;

struct FramePlaneView {
    const std::byte* data;   // Pointer to the beginning of this plane's memory region.
    size_t length;           // Total size of this plane's allocated memory region, in bytes.
    size_t bytesUsed;        // Number of valid bytes currently containing frame data.
    size_t stride;           // How many bytes you move in memory to go from one plane row to the next.
};

struct FrameData {
    uint64_t sequenceNumber;                // Number identifying this frame in the capture sequence.
    uint64_t timestampNs;                   // Timestamp expressed in nanoseconds
    uint32_t width;
    uint32_t height;
    std::string pixelFormat;                // RGB / YUW
    std::vector<FramePlaneView> planes;     // Memory views of all planes that make up this frame
};

struct CameraConfigSpec {
    libcamera::StreamRole role;
    // std::optional<uint32_t> width;
    // std::optional<uint32_t> height;
    // std::optional<std::string> pixelFormat;
    // std::optional<uint32_t> fps;
};


class CameraBackend {
public:
    using FrameCallback = std::function<void(const FrameData&)>;
    using ErrorCallback = std::function<void(const std::string&)>;

    CameraBackend() = default;
    ~CameraBackend();

    bool initialize();
    void shutdown();

    bool configureForPreview();
    bool configureForRecording();
    bool configureForSnapshot();

    // makes the configured camera pipeline actually begin producing frames.
    bool startCapture();
    bool stopCapture();

    void reportError(const std::string& error) const;

    void setFrameCallback(FrameCallback callback);
    void setErrorCallback(ErrorCallback callback);

private:
    std::unique_ptr<CameraManager> cameraManager_ { nullptr };
    std::shared_ptr<libcamera::Camera> camera_ { nullptr };
    std::unique_ptr<CameraConfiguration> activeConfiguration_ { nullptr };
    FrameCallback frameCallback_;
    ErrorCallback errorCallback_;
    bool initialized_ { false };
    Stream* activeStream_ { nullptr };
    std::unique_ptr<FrameBufferAllocator> bufferAllocator_;
    std::vector<std::unique_ptr<Request>> requests_;
    bool capturing_ { false };
    uint64_t frameSequenceCounter_ {0};

    // methods
    bool configureFromSpec(const CameraConfigSpec&);
    void onRequestCompleted(Request* request);
};


#endif //EMBEDDED_VEHICLE_STACK_CAMERABACKEND_H

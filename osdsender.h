#pragma once

#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <string>
#include "cef_includes.h"
#include "thrift-services/src-client/VdrClient.h"

// Sends the OSD to VDR in its own thread.
//
// OnPaint() runs in the UI thread of the browser. Sending synchronously blocked the
// browser (javascript, keys, the next frame) until VDR had drawn the image. Paint()
// only copies the changed regions into a frame buffer. The sender thread sends the
// latest state: frames which arrive while sending are merged.
class OsdSender {
public:
    OsdSender(VdrClient *client, bool qoi, bool measure = false);
    ~OsdSender();

    // called from OnPaint()
    void Paint(const void *buffer, int width, int height, const std::vector<CefRect> &rects);

    // measurement: a key has been sent to the page
    static void KeyPressed(const std::string &key);

    // VDR couldn't read the shared memory: try it again
    static void RetrySharedMemory();

private:
    void Run();
    void AddDirty(const CefRect &rect);
    void Send(int width, int height, const std::vector<CefRect> &rects, std::vector<std::string> &data);

    // The whole frame in a file of /dev/shm for VDR on the same machine: only the
    // changed regions are told, VDR reads them. Used by the sender thread only.
    bool OpenShm(int width, int height);
    void CloseShm();

    uint8_t *shm = nullptr;
    size_t shmSize = 0;
    std::string shmName;
    int shmGeneration = 0;

    VdrClient *vdrClient;
    bool useQoi;

    std::mutex mutex;
    std::condition_variable condition;
    bool stop = false;

    std::vector<uint32_t> frame;      // the last frame of the browser
    int frameWidth = 0;
    int frameHeight = 0;
    std::vector<CefRect> dirty;       // regions not sent yet

    // VDR shows a hint if the page shows nothing (all pixels transparent)
    size_t visiblePixels = 0;
    bool empty = true;
    bool emptyChanged = false;

    std::thread thread;

    // measurement (--measure)
    using clock = std::chrono::steady_clock;
    bool measure;
    clock::time_point firstPaint;          // first Paint() not sent yet
    clock::time_point keyPaint;            // first Paint() after a key
    std::string keyPaintName;
    clock::time_point statsStart;
    int statsFrames = 0;
    int statsRects = 0;
    size_t statsBytes = 0;
    double statsWait = 0;
    double statsSend = 0;
    double statsSendMax = 0;

    void Measure(clock::time_point waitStart, clock::time_point sendStart, int rects, size_t bytes);
};

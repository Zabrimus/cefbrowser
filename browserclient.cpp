#include "browserclient.h"
#include "sharedmemory.h"
#include "database.h"

BrowserClient::BrowserClient(bool fullscreen, int width, int height, std::string vdrIp, int vdrPort, std::string transcoderIp, int transcoderPort, std::string browserIp, int browserPort)
                    : vdrIp(vdrIp), vdrPort(vdrPort), transcoderIp(transcoderIp), transcoderPort(transcoderPort), browserIp(browserIp), browserPort(browserPort) {
    TRACE("BrowserClient::BrowserClient");
    this->renderWidth = width;
    this->renderHeight = height;

    // create clients
    transcoderRemoteClient = new TranscoderRemoteClient(transcoderIp, transcoderPort, browserIp, browserPort);
    vdrRemoteClient = new VdrRemoteClient(vdrIp, vdrPort);
}

BrowserClient::~BrowserClient() {
    TRACE("BrowserClient:~BrowserClient");

    delete transcoderRemoteClient;
    delete vdrRemoteClient;
}

void BrowserClient::osdClearVideo(int x, int y, int width, int height) {
    TRACE("BrowserClient::osdClearVideo: {}, {}, {}, {}", x, y, width, height);

    clearX = x;
    clearY = y;
    clearWidth = width;
    clearHeight = height;
}

void BrowserClient::setRenderSize(int width, int height) {
    TRACE("BrowserClient::setRenderSize: {}, {}", width, height);

    renderWidth = width;
    renderHeight = height;
}

void BrowserClient::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) {
    LOG_CURRENT_THREAD();

    rect = CefRect(0, 0, renderWidth, renderHeight);
}

void BrowserClient::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height) {
    LOG_CURRENT_THREAD();

    INFO("BrowserClient::OnPaint, width: {}, height: {}", width, height);

    sharedMemory.write((uint8_t *)buffer, width * height * 4);

    // hex = 0xAARRGGBB.
    // rgb(254, 46, 154) = #fe2e9a
    // fffe2e9a => 00fe2e9a

    int w = std::min(width, 1920);
    int h = std::min(height, 1080);

    // delete parts of the OSD where a video shall be visible
    uint32_t* buf = (uint32_t*)sharedMemory.get();
    for (uint32_t i = 0; i < (uint32_t)(width * height); ++i) {
        if (buf[i] == 0xfffe2e9a) {
            buf[i] = 0x00fe2e9a;
        }
    }

    vdrRemoteClient->ProcessOsdUpdate(width, height);
}

bool BrowserClient::GetAudioParameters(CefRefPtr<CefBrowser> browser, CefAudioParameters &params) {
    TRACE("BrowserClient::GetAudioParameters");

    auto ret = CefAudioHandler::GetAudioParameters(browser, params);

    // 48k default sample rate seems to better work with different input videos
    params.sample_rate = 48000;

    return ret;
}

void BrowserClient::OnAudioStreamStarted(CefRefPtr<CefBrowser> browser, const CefAudioParameters &params, int channels) {
    TRACE("BrowserClient::OnAudioStreamStarted: Sample rate: {}, Channel layout: {}, Frames per buffer: {}, Channels: {} ", params.sample_rate, (int)params.channel_layout, params.frames_per_buffer, channels);
}

void BrowserClient::OnAudioStreamPacket(CefRefPtr<CefBrowser> browser, const float **data, int frames, int64 pts) {
    TRACE("BrowserClient::OnAudioStreamPacket: Frames: {}, pts: {}", frames, pts);
}

void BrowserClient::OnAudioStreamStopped(CefRefPtr<CefBrowser> browser) {
    DEBUG("OSRVideoHandler::OnAudioStreamStopped");
    // this event will also be triggered if video paused. => do nothing
}

void BrowserClient::OnAudioStreamError(CefRefPtr<CefBrowser> browser, const CefString &message) {
    DEBUG("OSRVideoHandler::OnAudioStreamError: {}", message.ToString());
}

void BrowserClient::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
    TRACE("BrowserClient::OnAfterCreated");
}

void BrowserClient::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
}

bool BrowserClient::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message) {
    DEBUG("BrowserClient::OnProcessMessageReceived {}", message->GetName().ToString());

    if (message->GetName().ToString() == "RedButton") {
        if (message->GetArgumentList()->GetSize() == 1) {
            std::string channelId = message->GetArgumentList()->GetString(0).ToString();
            DEBUG("BrowserClient::OnProcessMessageReceived: RedButton {}", channelId);

            std::string url = database.getRedButtonUrl(channelId);
            std::string channel = database.getChannel(channelId);

            std::ofstream _dynamic;
            _dynamic.open ("js/_dynamic.js", std::ios_base::trunc);
            _dynamic << "window.HBBTV_POLYFILL_NS = window.HBBTV_POLYFILL_NS || {}; window.HBBTV_POLYFILL_NS.currentChannel = " << channel << std::endl;
            _dynamic.close();

            // load url
            browser->GetMainFrame()->LoadURL(url);

            return true;
        } else {
            ERROR("BrowserClient::OnProcessMessageReceived: RedButton without channelId");
        }
    } else if (message->GetName().ToString() == "LoadUrl") {
        if (message->GetArgumentList()->GetSize() == 1) {
            std::string url = message->GetArgumentList()->GetString(0).ToString();
            DEBUG("BrowserClient::OnProcessMessageReceived: LoadUrl {}", url);

            // load url
            browser->GetMainFrame()->LoadURL(url);

            return true;
        } else {
            ERROR("BrowserClient::OnProcessMessageReceived: RedButton without channelId");
        }
    }  else if (message->GetName().ToString() == "StartApp") {
        if (message->GetArgumentList()->GetSize() == 3) {
            const auto channelId = message->GetArgumentList()->GetString(0).ToString();
            const auto appId = message->GetArgumentList()->GetString(1).ToString();
            const auto args = message->GetArgumentList()->GetString(2).ToString();

            DEBUG("BrowserClient::OnProcessMessageReceived: StartApp {}, {}, {}", channelId, appId, args);

            // get AppUrl
            std::string url = database.getAppUrl(channelId, appId);
            if (!args.empty()) {
                url += args;
            }

            // load url
            browser->GetMainFrame()->LoadURL(url);

            return true;
        } else {
            ERROR("BrowserClient::OnProcessMessageReceived: RedButton without channelId");
        }
    }

    return false;
}

CefRefPtr<CefResourceRequestHandler> BrowserClient::GetResourceRequestHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                                CefRefPtr<CefRequest> request,
                                                bool is_navigation,
                                                bool is_download,
                                                const CefString &request_initiator,
                                                bool &disable_default_handling)
{
    std::string url = request->GetURL().ToString();

    if (!startsWith(url, "http://" + browserIp + ":" + std::to_string(browserPort) + "/application") && (startsWith(url, "http://localhost") || startsWith(url, "http://127.0.0.1") || startsWith(url, "http://" + browserIp))  ) {
        // let the browser handle this
        return nullptr;
    }

    if (is_navigation) {
        DEBUG("GetResourceRequestHandler: is_navigation:{}, URL:{}, Initiator:{}", is_navigation,
              request->GetURL().ToString(), request_initiator.ToString());
        return new RequestResponse(browser, frame, request, is_navigation, is_download, request_initiator,
                                   disable_default_handling, browserIp, browserPort);
    }

    return nullptr;
}


bool BrowserClient::OnBeforeBrowse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool user_gesture, bool is_redirect) {
    return false;
}

void
BrowserClient::OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser, CefRequestHandler::TerminationStatus status) {
}


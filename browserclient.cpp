#include <chrono>
#include <algorithm>
#include "browserclient.h"
#include "sharedmemory.h"
#include "database.h"
#include "tools.h"

#define QOI_IMPLEMENTATION
#include "qoi.h"

#define ONPAINT_MEASURE_TIME 0

std::string urlBlockList[] {
        ".block.this",
        ".nmrodam.com",
        ".ioam.de",
        ".xiti.com",
        ".sensic.net",
        ".tvping.com",
        "tracking.redbutton.de",
        "px.moatads.com",
        "2mdn.net",
        ".gvt1.com", // testwise
        ".gvt2.com"  // testwise
};

BrowserClient::BrowserClient(bool fullscreen, int width, int height, std::string vdrIp, int vdrPort, std::string transcoderIp, int transcoderPort, std::string browserIp, int browserPort, image_type_enum osdqoi, bool use_dirty_recs)
                    : vdrIp(vdrIp), vdrPort(vdrPort), transcoderIp(transcoderIp), transcoderPort(transcoderPort), browserIp(browserIp), browserPort(browserPort), osdqoi(osdqoi), use_dirty_recs(use_dirty_recs) {
    LOG_CURRENT_THREAD();

    this->renderWidth = width;
    this->renderHeight = height;

    // create clients
    transcoderRemoteClient = new TranscoderRemoteClient(transcoderIp, transcoderPort, browserIp, browserPort);
    vdrRemoteClient = new VdrRemoteClient(vdrIp, vdrPort);
}

BrowserClient::~BrowserClient() {
    LOG_CURRENT_THREAD();

    delete transcoderRemoteClient;
    delete vdrRemoteClient;
}

void BrowserClient::setRenderSize(int width, int height) {
    LOG_CURRENT_THREAD();
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

    if (width > renderWidth || height > renderHeight) {
        CRITICAL("BrowserClient::OnPaint, width {}, height {} are too large. Ignore this request.");
        return;
    }

    RectList recList;
    if (use_dirty_recs) {
        recList = dirtyRects;
    } else {
        CefRect rect(0, 0, width, height);
        recList.emplace_back(rect);
    }

    // iterate overall dirty recs
    for (auto r : recList) {
        uint32_t* outbuffer = new uint32_t[r.width * r.height];

        // copy the region
        uint32_t* ci = (uint32_t *) buffer + (r.y * width + r.x);
        uint32_t* co = (uint32_t *) outbuffer;

        for (int dry = 0; dry < r.height; ++dry) {
            memcpy(co, ci, r.width * 4);
            ci += width;
            co += r.width;
        }

        // delete parts of the OSD where a video shall be visible
        for (uint32_t i = 0; i < (uint32_t) (r.width * r.height); ++i) {
            if (outbuffer[i] == 0xfffe2e9a) {
                outbuffer[i] = 0x00fe2e9a;
            }
        }

        if (osdqoi == QOI) {
            /*
            // encode qoi image
            for (int i = 0; i < r.width * r.height; ++i) {
                // Source: BGRA = 0xAARRGGBB
                // Dest:   RGBA = 0xAABBGGRR
                outbuffer[i] =
                        ((outbuffer[i] & 0xFF00FF00)) | // AA__GG__
                        ((outbuffer[i] & 0x00FF0000) >> 16) | // __RR____ -> ______RR
                        ((outbuffer[i] & 0x000000FF) << 16);  // ______BB -> __BB____
            }
            */
#if ONPAINT_MEASURE_TIME == 1
            auto begin = std::chrono::high_resolution_clock::now();
#endif

            qoi_desc desc{
                    .width = static_cast<unsigned int>(r.width),
                    .height = static_cast<unsigned int>(r.height),
                    .channels = 4,
                    .colorspace = QOI_LINEAR
            };

            int out_len;
            char *encoded_image = static_cast<char *>(qoi_encode(outbuffer, &desc, &out_len));

#if ONPAINT_MEASURE_TIME == 1
            auto end = std::chrono::high_resolution_clock::now();
            std::cout << "OnPaint QOI Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()
                      << "ms, size " << out_len << std::endl;
#endif

            if (!vdrRemoteClient->ProcessOsdUpdateQoi(renderWidth, renderHeight, r.x, r.y, std::string(encoded_image, out_len))) {
                // OSD in VDR is not available
                loadUrl(browser, "about:blank");
                sharedMemory.Clear();
            }

            free(encoded_image);

        } else {
            sharedMemory.Write((uint8_t *)outbuffer, r.width * r.height * 4);
            if (!vdrRemoteClient->ProcessOsdUpdate(renderWidth, renderHeight, r.x, r.y, r.width, r.height)) {
                // OSD in VDR is not available
                loadUrl(browser, "about:blank");
                sharedMemory.Clear();
            }
        }

        delete[] outbuffer;
    }
}

void BrowserClient::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
    LOG_CURRENT_THREAD();
    TRACE("BrowserClient::OnAfterCreated");
}

void BrowserClient::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
    LOG_CURRENT_THREAD();
    TRACE("BrowserClient::OnBeforeClose");
}

bool BrowserClient::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message) {
    LOG_CURRENT_THREAD();
    TRACE("BrowserClient::OnProcessMessageReceived {}", message->GetName().ToString());

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

            loadUrl(browser, url);
            return true;
        } else {
            ERROR("BrowserClient::OnProcessMessageReceived: RedButton without channelId");
        }
    } else if (message->GetName().ToString() == "LoadUrl") {
        if (message->GetArgumentList()->GetSize() == 1) {
            std::string url = message->GetArgumentList()->GetString(0).ToString();
            DEBUG("BrowserClient::OnProcessMessageReceived: LoadUrl {}", url);

            loadUrl(browser, url);
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
            if (url.empty()) {
                INFO("Application with appId {} for channelId {} not found -> ignore request");
                return false;
            }

            if (!args.empty()) {
                url += args;
            }

            loadUrl(browser, url);
            return true;
        } else {
            ERROR("BrowserClient::OnProcessMessageReceived: StartApp without channelId or appId");
        }
    } else if (message->GetName().ToString() == "SetDirtyOSD") {
        browser->GetHost()->Invalidate(PET_VIEW);
    }

    return false;
}

void BrowserClient::loadUrl(CefRefPtr<CefBrowser> browser, const std::string& url) {
    // Before loading a new url stop video (in VDR and transcoder) and set fullscreen

    // TODO:
    //  Bei manchen Seiten (z.B. ARD/Tagesschau) ist bei einem Wechsel des Videos innerhalb Seite kein StopVideo/Videofullscreen nötig.
    //  Es reicht, den Transcoder zu stoppen, obwohl selbst das muss nicht sein - denke ich - da der Transcoder das selbst managed.

    vdrRemoteClient->StopVideo();

    std::string reason = "BrowserClient::loadUrl";
    transcoderRemoteClient->Stop(reason);

    // load url
    browser->GetMainFrame()->LoadURL(url);
}

CefRefPtr<CefResourceRequestHandler> BrowserClient::GetResourceRequestHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                                CefRefPtr<CefRequest> request,
                                                bool is_navigation,
                                                bool is_download,
                                                const CefString &request_initiator,
                                                bool &disable_default_handling)
{
    LOG_CURRENT_THREAD();

    std::string url = request->GetURL().ToString();

    if (!startsWith(url, "http://" + browserIp + ":" + std::to_string(browserPort) + "/application") && (startsWith(url, "http://localhost") || startsWith(url, "http://127.0.0.1") || startsWith(url, "http://" + browserIp))  ) {
        // let the browser handle this
        return nullptr;
    }

    // check if URL has to be blocked
    bool blockThis = false;
    size_t found = url.find_first_of(':');
    size_t found2 = url.substr(found + 3).find_first_of('/');

    for (const auto & block : urlBlockList) {
        if (url.substr(found + 3, found2).find(block) != std::string::npos) {
            blockThis = true;
        }
    }

    if (!blockThis) {
        TRACE("GetResourceRequestHandler: is_navigation:{}, blockThis:{}, URL:{}, Initiator:{}", is_navigation,
              blockThis, request->GetURL().ToString(), request_initiator.ToString());

        if (request->GetResourceType() == RT_MEDIA) {
            TRACE("GetResourceRequestHandler: RT_MEDIA: {}", request->GetURL().ToString());
        }
    }

    /*
    if (logger->isTraceEnabled()) {
        switch(request->GetResourceType()) {
            case RT_MAIN_FRAME:
                TRACE("GetResourceRequestHandler: RT_MAIN_FRAME");
                break;
            case RT_SUB_FRAME:
                TRACE("GetResourceRequestHandler: RT_SUB_FRAME");
                break;
            case RT_STYLESHEET:
                TRACE("GetResourceRequestHandler: RT_STYLESHEET");
                break;
            case RT_SCRIPT:
                TRACE("GetResourceRequestHandler: RT_SCRIPT");
                break;
            case RT_IMAGE:
                TRACE("GetResourceRequestHandler: RT_IMAGE");
                break;
            case RT_FONT_RESOURCE:
                TRACE("GetResourceRequestHandler: RT_FONT_RESOURCE");
                break;
            case RT_SUB_RESOURCE:
                TRACE("GetResourceRequestHandler: RT_SUB_RESOURCE");
                break;
            case RT_OBJECT:
                TRACE("GetResourceRequestHandler: RT_OBJECT");
                break;
            case RT_MEDIA:
                TRACE("GetResourceRequestHandler: RT_MEDIA: {}", request->GetURL().ToString());
                break;
            case RT_WORKER:
                TRACE("GetResourceRequestHandler: RT_WORKER");
                break;
            case RT_SHARED_WORKER:
                TRACE("GetResourceRequestHandler: RT_SHARED_WORKER");
                break;
            case RT_PREFETCH:
                TRACE("GetResourceRequestHandler: RT_PREFETCH");
                break;
            case RT_FAVICON:
                TRACE("GetResourceRequestHandler: RT_FAVICON");
                break;
            case RT_XHR:
                TRACE("GetResourceRequestHandler: RT_XHR");
                break;
            case RT_PING:
                TRACE("GetResourceRequestHandler: RT_PING");
                break;
            case RT_SERVICE_WORKER:
                TRACE("GetResourceRequestHandler: RT_SERVICE_WORKER");
                break;
            case RT_CSP_REPORT:
                TRACE("GetResourceRequestHandler: RT_CSP_REPORT");
                break;
            case RT_PLUGIN_RESOURCE:
                TRACE("GetResourceRequestHandler: RT_PLUGIN_RESOURCE");
                break;
            case RT_NAVIGATION_PRELOAD_MAIN_FRAME:
                TRACE("GetResourceRequestHandler: RT_NAVIGATION_PRELOAD_MAIN_FRAME");
                break;
            case RT_NAVIGATION_PRELOAD_SUB_FRAME:
                TRACE("GetResourceRequestHandler: RT_NAVIGATION_PRELOAD_SUB_FRAME");
                break;
        }
    }
    */

    if ((is_navigation && request->GetResourceType() != RT_SUB_FRAME) || (request->GetResourceType() == RT_SUB_RESOURCE) || blockThis) {
        if (!blockThis) {
            DEBUG("GetResourceRequestHandler: is_navigation:{}, URL:{}, Initiator:{}", is_navigation,
                  request->GetURL().ToString(), request_initiator.ToString());
        }

        return new RequestResponse(browser, frame, request, is_navigation, is_download, request_initiator,
                                   disable_default_handling, browserIp, browserPort, blockThis);
    }

    return nullptr;
}


bool BrowserClient::OnBeforeBrowse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool user_gesture, bool is_redirect) {
    DEBUG("BrowserClient::OnBeforeBrowse: Url: {}, Redirect: {}, UserGesture: {}", request->GetURL().ToString(), is_redirect, user_gesture);

    if (request->GetURL().ToString().find("p7s1video.net/") != std::string::npos) {
        DEBUG("Found external video domain. Cancel the request. Request will be handled later.");
        return true;
    }

    return false;
}

void BrowserClient::OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser, CefRequestHandler::TerminationStatus status) {
}

void BrowserClient::OnStatusMessage(CefRefPtr<CefBrowser> browser, const CefString &value) {
    TRACE("[JS] StatusMessage: {}", value.ToString());
}

bool BrowserClient::OnConsoleMessage(CefRefPtr<CefBrowser> browser, cef_log_severity_t level, const CefString &message, const CefString &source, int line) {
    if (!logger->isTraceEnabled()) {
        return false;
    }

    std::string log_message;

    switch(level) {
        case LOGSEVERITY_DEFAULT:
            log_message = "[Default] ";
            break;
        case LOGSEVERITY_VERBOSE:
            log_message = "[Verbose] ";
            break;
        case LOGSEVERITY_INFO:
            log_message = "[Info] ";
            break;
        case LOGSEVERITY_WARNING:
            log_message = "[Warning] ";
            break;
        case LOGSEVERITY_ERROR:
            log_message = "[Error] ";
            break;
        case LOGSEVERITY_FATAL:
            log_message = "[Fatal] ";
            break;
        case LOGSEVERITY_DISABLE:
            log_message = "[Disabled] ";
            break;
    }

    log_message += message.ToString() + " <" + source.ToString() + ", " + std::to_string(line) + ">";

    JSTRACE("[JS] {}", log_message);

    return false;
}


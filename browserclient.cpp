#include <chrono>
#include <algorithm>
#include "browserclient.h"
#include "database.h"
#include "tools.h"
#include "moviestream.h"
#include "httpinterception.h"
#include "xhrinterception.h"
#include "trackinginterception.h"
#include "statichandler.h"

#define QOI_IMPLEMENTATION
#include "qoi.h"

#define ONPAINT_MEASURE_TIME 0

extern scoped_refptr<CefBrowser> currentBrowser;

BrowserClient::BrowserClient(bool fullscreen, BrowserParameter bp) : bParam(bp) {
    LOG_CURRENT_THREAD();

    this->renderWidth = bParam.zoom_width;
    this->renderHeight = bParam.zoom_height;
    this->processorEnabled = true;

    // create clients
    transcoderClient = new TranscoderClient(bParam.transcoderIp, bParam.transcoderPort);
    vdrClient = new VdrClient(bParam.vdrIp, bParam.vdrPort);

    streamId = bParam.browserIp + "_" + std::to_string(bParam.browserPort);
}

BrowserClient::~BrowserClient() {
    LOG_CURRENT_THREAD();

    delete transcoderClient;
    transcoderClient = nullptr;

    delete vdrClient;
    vdrClient = nullptr;
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
    if (bParam.use_dirty_recs) {
        recList = dirtyRects;
    } else {
        CefRect rect(0, 0, width, height);
        recList.emplace_back(rect);
    }

#if ONPAINT_MEASURE_TIME == 1
    auto begin = std::chrono::high_resolution_clock::now();
#endif

    // iterate overall dirty recs
    for (auto r : recList) {
        uint32_t* outbuffer = new uint32_t[r.width * r.height];
        memset(outbuffer, 0, r.width * r.height);

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

        if (bParam.osdqoi == QOI) {
            qoi_desc desc{
                    .width = static_cast<unsigned int>(r.width),
                    .height = static_cast<unsigned int>(r.height),
                    .channels = 4,
                    .colorspace = QOI_LINEAR
            };

            int out_len = 0;
            char *encoded_image = static_cast<char *>(qoi_encode(outbuffer, &desc, &out_len));

            vdrClient->ProcessOsdUpdateQoi(renderWidth, renderHeight, r.x, r.y, std::string(encoded_image, out_len));
            free(encoded_image);
        } else {
            std::string data = std::string((char *)outbuffer, r.width * r.height * 4);
            vdrClient->ProcessOsdUpdate(renderWidth, renderHeight, r.x, r.y, r.width, r.height, data);
        }

        delete[] outbuffer;
    }

#if ONPAINT_MEASURE_TIME == 1
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "OnPaint Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()
                      << "ms" << std::endl;
#endif

}

void BrowserClient::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
    LOG_CURRENT_THREAD();
    TRACE("BrowserClient::OnAfterCreated");

    CefRefPtr<DevToolsMessageObserver> devToolsObserver = new DevToolsMessageObserver();
    registration = browser->GetHost()->AddDevToolsMessageObserver(devToolsObserver);
}

void BrowserClient::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
    LOG_CURRENT_THREAD();
    TRACE("BrowserClient::OnBeforeClose");

    if (browser->IsSame(currentBrowser)) {
        currentBrowser = nullptr;
    }
}

bool BrowserClient::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message) {
    LOG_CURRENT_THREAD();
    TRACE("BrowserClient::OnProcessMessageReceived {}", message->GetName().ToString());

    if (message->GetName().ToString() == "RedButton") {
        if (message->GetArgumentList()->GetSize() == 1) {
            std::string channelId = message->GetArgumentList()->GetString(0).ToString();
            DEBUG("BrowserClient::OnProcessMessageReceived: RedButton {}", channelId);

            std::string url = database->getRedButtonUrl(channelId);
            std::string channel = database->getChannel(channelId);

            std::ofstream _dynamic;
            _dynamic.open (bParam.static_path + "/js/_dynamic.js", std::ios_base::trunc);
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
            ERROR("BrowserClient::OnProcessMessageReceived: LoadUrl without an URL");
        }
    }  else if (message->GetName().ToString() == "StartApp") {
        if (message->GetArgumentList()->GetSize() == 3) {
            const auto channelId = message->GetArgumentList()->GetString(0).ToString();
            const auto appId = message->GetArgumentList()->GetString(1).ToString();
            const auto args = message->GetArgumentList()->GetString(2).ToString();

            DEBUG("BrowserClient::OnProcessMessageReceived: StartApp {}, {}, {}", channelId, appId, args);

            // get AppUrl
            std::string url = database->getAppUrl(channelId, appId);
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

    vdrClient->StopVideo();

    std::string reason = "BrowserClient::loadUrl";
    transcoderClient->Stop(streamId, reason);

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

    static std::string browserUrl = std::string("http://") + bParam.browserIp + std::string(":") + std::to_string(bParam.browserPort);

    std::string url = request->GetURL().ToString();

    bool blockThis = TrackingInterception::IsTracker(url);

    if (logger->isTraceEnabled() && !blockThis) {
        TRACE("Request URL: {}", url);
    }

    if (url.find("amazon.de") != std::string::npos) {
        return nullptr;
    }

    // let cef handle the whole URL loading. For HbbTV pages, this does not makes sense.
    if (!processorEnabled) {
        return nullptr;
    }

    /* */
    if (logger->isTraceEnabled() && !blockThis) {
        switch(request->GetResourceType()) {
            case RT_MAIN_FRAME:
                TRACE("GetResourceRequestHandler: RT_MAIN_FRAME, Method: {}", request->GetMethod().ToString());
                break;
            case RT_SUB_FRAME:
                TRACE("GetResourceRequestHandler: RT_SUB_FRAME, Method: {}", request->GetMethod().ToString());
                break;
            case RT_STYLESHEET:
                TRACE("GetResourceRequestHandler: RT_STYLESHEET, Method: {}", request->GetMethod().ToString());
                break;
            case RT_SCRIPT:
                TRACE("GetResourceRequestHandler: RT_SCRIPT, Method: {}", request->GetMethod().ToString());
                break;
            case RT_IMAGE:
                TRACE("GetResourceRequestHandler: RT_IMAGE, Method: {}", request->GetMethod().ToString());
                break;
            case RT_FONT_RESOURCE:
                TRACE("GetResourceRequestHandler: RT_FONT_RESOURCE, Method: {}", request->GetMethod().ToString());
                break;
            case RT_SUB_RESOURCE:
                TRACE("GetResourceRequestHandler: RT_SUB_RESOURCE, Method: {}", request->GetMethod().ToString());
                break;
            case RT_OBJECT:
                TRACE("GetResourceRequestHandler: RT_OBJECT, Method: {}", request->GetMethod().ToString());
                break;
            case RT_MEDIA:
                TRACE("GetResourceRequestHandler: RT_MEDIA: {}, Method: {}", request->GetURL().ToString(), request->GetMethod().ToString());
                break;
            case RT_WORKER:
                TRACE("GetResourceRequestHandler: RT_WORKER, Method: {}", request->GetMethod().ToString());
                break;
            case RT_SHARED_WORKER:
                TRACE("GetResourceRequestHandler: RT_SHARED_WORKER, Method: {}", request->GetMethod().ToString());
                break;
            case RT_PREFETCH:
                TRACE("GetResourceRequestHandler: RT_PREFETCH, Method: {}", request->GetMethod().ToString());
                break;
            case RT_FAVICON:
                TRACE("GetResourceRequestHandler: RT_FAVICON, Method: {}", request->GetMethod().ToString());
                break;
            case RT_XHR:
                TRACE("GetResourceRequestHandler: RT_XHR, Method: {}", request->GetMethod().ToString());
                break;
            case RT_PING:
                TRACE("GetResourceRequestHandler: RT_PING, Method: {}", request->GetMethod().ToString());
                break;
            case RT_SERVICE_WORKER:
                TRACE("GetResourceRequestHandler: RT_SERVICE_WORKER, Method: {}", request->GetMethod().ToString());
                break;
            case RT_CSP_REPORT:
                TRACE("GetResourceRequestHandler: RT_CSP_REPORT, Method: {}", request->GetMethod().ToString());
                break;
            case RT_PLUGIN_RESOURCE:
                TRACE("GetResourceRequestHandler: RT_PLUGIN_RESOURCE, Method: {}", request->GetMethod().ToString());
                break;
            case RT_NAVIGATION_PRELOAD_MAIN_FRAME:
                TRACE("GetResourceRequestHandler: RT_NAVIGATION_PRELOAD_MAIN_FRAME, Method: {}", request->GetMethod().ToString());
                break;
            case RT_NAVIGATION_PRELOAD_SUB_FRAME:
                TRACE("GetResourceRequestHandler: RT_NAVIGATION_PRELOAD_SUB_FRAME, Method: {}", request->GetMethod().ToString());
                break;
        }
    }

    /* Tracking */
    if (blockThis || url == BLANK_PAGE) {
        return new TrackingInterception(request->GetResourceType());
    }

    /* internal video loading */
    if (url.find("http://localhost/movie/") != std::string::npos) {
        return new MovieStream(transcoderClient);
    }

    /* static content */
    if (url.find(browserUrl) != std::string::npos) {
        return new StaticHandler(bParam.static_path, url.substr(browserUrl.length(), url.length()-browserUrl.length()));
    }

    if (url.find("http://localhost/") != std::string::npos) {
        int length = strlen("http://localhost");
        return new StaticHandler(bParam.static_path, url.substr(length, url.length()-length));
    }

    /* Special handling for some requests. xhook replacement */
    if ((request->GetResourceType()) == RT_XHR &&
            ((url.find("-hbbtv.zdf.de/al/cms/content/") != std::string::npos) ||
             (url.find("hbbtv.zdf.de/zdfm3/dyn/get.php") != std::string::npos) ||
             (url.find("-hbbtv.zdf.de/ds/configuration") != std::string::npos) ||
             (url.find("tv.ardmediathek.de/dyn/get?id=video") != std::string::npos)
             )) {
        return new XhrInterception();
    }

    /* Main Frame */
    if (is_navigation && request->GetResourceType() == RT_MAIN_FRAME) {
        return new HttpInterception(bParam.static_path, blockThis);
    }

    TRACE("No special handler for request URL: {}", url);

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

void BrowserClient::OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser, TerminationStatus status, int error_code, const CefString& error_string) {
    CRITICAL("[Crash] Render process terminated, errorcode: {}, error message: {}", error_code, error_string.ToString());
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

// CefFrameHandler
void BrowserClient::OnFrameCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame) {
    TRACE("BrowserClient::OnFrameCreated");
}

void BrowserClient::OnFrameAttached(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, bool reattached) {
    TRACE("BrowserClient::OnFrameAttached");
}

void BrowserClient::OnFrameDetached(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame) {
    TRACE("BrowserClient::OnFrameDetached");
}

void BrowserClient::OnMainFrameChanged(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> old_frame, CefRefPtr<CefFrame> new_frame) {
    TRACE("BrowserClient::OnMainFrameChanged, old_frame {}", old_frame != nullptr ? "present" : "isNull");
}

void BrowserClient::ChangeUserAgent(CefRefPtr<CefBrowser> browser, std::string agent) {
    DEBUG("Change User Agent to {}", agent);

    CefRefPtr<CefDictionaryValue> params = CefDictionaryValue::Create();
    params->SetString("userAgent", agent);

    int status = browser->GetHost()->ExecuteDevToolsMethod(0, "Network.setUserAgentOverride", params);
    DEBUG("Status: {}", status);
}

void BrowserClient::enableProcessing(bool processUrl) {
    processorEnabled = processUrl;
}

bool DevToolsMessageObserver::OnDevToolsMessage(CefRefPtr<CefBrowser> browser, const void* message, size_t message_size) {
    std::string m;
    m.assign((const char*)message, message_size);

    DEBUG("DevToolsMessageObserver::OnDevToolsMessage: message {}", m);

    return false;
}

void DevToolsMessageObserver::OnDevToolsMethodResult(CefRefPtr<CefBrowser> browser, int message_id, bool success, const void* result, size_t result_size) {
    std::string m;
    m.assign((const char*)result, result_size);

    DEBUG("DevToolsMessageObserver::OnDevToolsMethodResult: id {}, success {}, message {}", message_id, success, m);
}

void DevToolsMessageObserver::OnDevToolsEvent(CefRefPtr<CefBrowser> browser, const CefString& method, const void* params, size_t params_size) {
    DEBUG("DevToolsMessageObserver::OnDevToolsEvent: method {}", method.ToString());
}

void DevToolsMessageObserver::OnDevToolsAgentAttached(CefRefPtr<CefBrowser> browser) {
    DEBUG("DevToolsMessageObserver::OnDevToolsAgentAttached");
}

void DevToolsMessageObserver::OnDevToolsAgentDetached(CefRefPtr<CefBrowser> browser) {
    DEBUG("DevToolsMessageObserver::OnDevToolsAgentDetached");
}

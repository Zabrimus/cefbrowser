#include "mainapp.h"

#include <utility>
#include "browserclient.h"
#include "v8handler.h"
#include "tools.h"
#include "keycodes.h"
#include "database.h"

scoped_refptr<CefBrowser> currentBrowser;

httplib::Server svr;
std::mutex httpServerMutex;

std::string lastInsertChannel = "";

void startHttpServer(std::string browserIp, int browserPort, std::string vdrIp, int vdrPort, std::string transcoderIp, int transcoderPort, std::string static_path, bool bindAll) {
    int _browserPort = browserPort;
    std::string _browserIp = browserIp;
    VdrRemoteClient vdrRemoteClient(vdrIp, vdrPort);
    TranscoderRemoteClient transcoderRemoteClient(transcoderIp, transcoderPort, browserIp, browserPort, vdrIp, vdrPort);

    if (static_path.empty()) {
        static_path = ".";
    }

    DEBUG("Mount static path {}", static_path);

    auto ret = svr.set_mount_point("/js", static_path + "/js");
    if (!ret) {
        // must not happen
        ERROR("http mount point {}/js does not exists. Application will not work as desired.", static_path);
        return;
    }

    ret = svr.set_mount_point("/css", static_path + "/css");
    if (!ret) {
        // must not happen
        ERROR("http mount point {}/css does not exists. Application will not work as desired.", static_path);
        return;
    }

    ret = svr.set_mount_point("/application", static_path + "/application");
    if (!ret) {
        // must not happen
        ERROR("http mount point {}/application does not exists. Application will not work as desired.", static_path);
        return;
    }

    // called by VDR
    svr.Post("/LoadUrl", [&transcoderRemoteClient](const httplib::Request &req, httplib::Response &res) {
        std::lock_guard<std::mutex> guard(httpServerMutex);

        auto url = req.get_param_value("url");
        INFO("Load URL: {}", url);

        if (url == "about:blank") {
            // special case
            transcoderRemoteClient.Stop(url);
        }

        if (url.empty()) {
            res.status = 404;
        } else {
            if (currentBrowser->GetMainFrame() != nullptr) { // Why is it possible, that MainFrame is null?
                currentBrowser->GetMainFrame()->LoadURL(url);
            } else {
                ERROR("MainFrame is null");
            }
            res.set_content("ok", "text/plain");
        }
    });

    svr.Post("/RedButton", [static_path](const httplib::Request &req, httplib::Response &res) {
        std::lock_guard<std::mutex> guard(httpServerMutex);

        auto channelId = req.get_param_value("channelId");
        INFO("Red Button for channelId: {}", channelId);

        if (channelId.empty()) {
            res.status = 404;
        } else {
            std::string url;

            // iterate multiple times.
            for (int i = 0; i < 5; ++i) {
                url = database.getRedButtonUrl(channelId);
                if (!url.empty()) {
                    break;
                }

                DEBUG("RedButton currently not available. Wait 250ms for a retry.");
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
            }

            if (url.empty()) {
                INFO("RedButton for channelId '{}' finally not found. Sent 404.", channelId);
                res.status = 404;
            } else {
                DEBUG("RedButton URL found: {}", url);

                // write channel information
                std::string channel = database.getChannel(channelId);
                std::ofstream _dynamic;
                _dynamic.open (static_path + "/js/_dynamic.js", std::ios_base::trunc);
                _dynamic << "window.HBBTV_POLYFILL_NS = window.HBBTV_POLYFILL_NS || {}; window.HBBTV_POLYFILL_NS.currentChannel = " << channel << std::endl;
                _dynamic.close();

                CefRefPtr<CefClient> currentClient = currentBrowser->GetHost()->GetClient();
                auto c = dynamic_cast<BrowserClient *>(currentClient.get());

                std::string newUserAgent = database.getUserAgent(channelId);
                INFO("Use UserAgent {} for {}", newUserAgent, channelId);
                c->ChangeUserAgent(currentBrowser, newUserAgent);

                // load url
                if (currentBrowser->GetMainFrame() != nullptr) { // Why is it possible, that MainFrame is null?
                    currentBrowser->GetMainFrame()->LoadURL(url);
                } else {
                    ERROR("MainFrame is null");
                }

                res.set_content("ok", "text/plain");
            }
        }
    });

    svr.Get("/ReloadOSD", [](const httplib::Request &req, httplib::Response &res) {
        currentBrowser->GetHost()->Invalidate(PET_VIEW);

        res.status = 200;
        res.set_content("ok", "text/plain");
    });

    svr.Get("/Heartbeat", [](const httplib::Request &req, httplib::Response &res) {
        res.status = 200;
        res.set_content("ok", "text/plain");
    });

    svr.Post("/StartApplication", [_browserIp, _browserPort, static_path](const httplib::Request &req, httplib::Response &res) {
        std::lock_guard<std::mutex> guard(httpServerMutex);

        auto channelId = req.get_param_value("channelId");
        auto appId = req.get_param_value("appId");
        INFO("Start Application, channelId {}, appId {}", channelId, appId);

        if (channelId.empty()) {
            res.status = 404;
        } else {
            // write channel information
            std::string channel = database.getChannel(channelId);
            std::ofstream _dynamic;
            _dynamic.open (static_path + "/js/_dynamic.js", std::ios_base::trunc);
            _dynamic << "window.HBBTV_POLYFILL_NS = window.HBBTV_POLYFILL_NS || {}; window.HBBTV_POLYFILL_NS.currentChannel = " << channel << std::endl;
            _dynamic.close();

            // create application url
            std::string url = "http://" + _browserIp + ":" + std::to_string(_browserPort) + "/application/main/main.html";

            // load url
            currentBrowser->GetMainFrame()->LoadURL(url);
            res.set_content("ok", "text/plain");
        }
    });

    // called by VDR
    svr.Post("/ProcessKey", [](const httplib::Request &req, httplib::Response &res) {
        std::lock_guard<std::mutex> guard(httpServerMutex);

        auto key = req.get_param_value("key");
        DEBUG("ProcessKey: {}", key);

        if (key.empty()) {
            res.status = 404;
        } else {
            if ((key.find("VK_BACK") != std::string::npos) ||
                (key.find("VK_RED") != std::string::npos) ||
                (key.find("VK_GREEN") != std::string::npos) ||
                (key.find("VK_YELLOW") != std::string::npos) ||
                (key.find("VK_BLUE") != std::string::npos) ||
                (key.find("VK_PLAY") != std::string::npos) ||
                (key.find("VK_PAUSE") != std::string::npos) ||
                (key.find("VK_STOP") != std::string::npos) ||
                (key.find("VK_FAST_FWD") != std::string::npos) ||
                (key.find("VK_REWIND") != std::string::npos)) {
                // send key event via Javascript
                std::ostringstream stringStream;
                stringStream << "window.cefKeyPress('" << key << "');";
                auto script = stringStream.str();
                auto frame = currentBrowser->GetMainFrame();

                if (frame != nullptr) { // Why is it possiböe, that MainFrame is null?
                    frame->ExecuteJavaScript(script, frame->GetURL(), 0);
                } else {
                    ERROR("MainFrame is null");
                }
            } else {
                // send key event via code
                // FIXME: Was ist mit keyCodes, die nicht existieren?
                int windowsKeyCode = keyCodes[key];
                CefKeyEvent keyEvent;

                keyEvent.windows_key_code = windowsKeyCode;
                keyEvent.modifiers = 0x00;

                keyEvent.type = KEYEVENT_RAWKEYDOWN;
                currentBrowser->GetHost()->SendKeyEvent(keyEvent);

                keyEvent.type = KEYEVENT_CHAR;
                currentBrowser->GetHost()->SendKeyEvent(keyEvent);

                keyEvent.type = KEYEVENT_KEYUP;
                currentBrowser->GetHost()->SendKeyEvent(keyEvent);
            }

            res.set_content("ok", "text/plain");
        }
    });

    // called by transcoder
    svr.Post("/ProcessTSPacket", [&vdrRemoteClient, &transcoderRemoteClient](const httplib::Request &req, httplib::Response &res) {
        std::lock_guard<std::mutex> guard(httpServerMutex);

        const std::string body = req.body;

        if (body.empty()) {
            res.status = 404;
        } else {
            // TRACE("ProcessTSPacket, length {}", body.length());

            if (!vdrRemoteClient.ProcessTSPacket(std::string(body.c_str(), body.length()))) {
                // vdr is not running? stop transcoder
                ERROR("vdr is not running? stop transcoder");
                std::string reason = "ProcessTSPacket failed";
                transcoderRemoteClient.Stop(reason);
            }

            res.set_content("ok", "text/plain");
        }
    });

    svr.Post("/StreamError", [&transcoderRemoteClient](const httplib::Request &req, httplib::Response &res) {
        std::lock_guard<std::mutex> guard(httpServerMutex);

        auto reason = req.get_param_value("reason");

        ERROR("Transcoder returned a stream error: {}", reason);

        std::string script1 = "document.getElementsByTagName('video')[0].currentTime = document.getElementsByTagName('video')[0].duration;";
        currentBrowser->GetMainFrame()->ExecuteJavaScript(script1, currentBrowser->GetMainFrame()->GetURL(), 0);

        std::string script2 = "window.cefKeyPress('VK_BACK');";
        currentBrowser->GetMainFrame()->ExecuteJavaScript(script2, currentBrowser->GetMainFrame()->GetURL(), 0);

        std::string reas = "stream error";
        transcoderRemoteClient.Stop(reas);
        // vdrRemoteClient.StopVideo();

        res.set_content("ok", "text/plain");
    });

    // called by VDR
    svr.Post("/InsertHbbtv", [](const httplib::Request &req, httplib::Response &res) {
        std::lock_guard<std::mutex> guard(httpServerMutex);

        const std::string body = req.body;

        if (body.empty()) {
            res.status = 404;
        } else {
            // TRACE("InsertHbbtv: {}", body);

            database.insertHbbtv(body);

            res.set_content("ok", "text/plain");
        }
    });

    // called by VDR
    svr.Post("/InsertChannel", [&transcoderRemoteClient](const httplib::Request &req, httplib::Response &res) {
        std::lock_guard<std::mutex> guard(httpServerMutex);

        const std::string body = req.body;

        if (body.empty()) {
            res.status = 404;
        } else {
            // TRACE("InsertChannel: {}", body);

            if (body != lastInsertChannel) {
                // in case of channel switch always stop playing videos
                std::string reason = "ChannelSwitch";
                transcoderRemoteClient.Stop(reason);
                lastInsertChannel = body;
            }

            database.insertChannel(body);

            res.set_content("ok", "text/plain");
        }
    });

    svr.Get("/StopVideo", [&transcoderRemoteClient](const httplib::Request &req, httplib::Response &res) {
        std::string reason = "VDR request StopVideo";
        transcoderRemoteClient.Stop(reason);

        res.status = 200;
        res.set_content("ok", "text/plain");
    });

    svr.set_exception_handler([](const auto& req, auto& res, std::exception_ptr ep) {
        auto fmt = "<h1>Error 500</h1><p>%s</p>";
        char buf[BUFSIZ];
        try {
            std::rethrow_exception(ep);
        } catch (std::exception &e) {
            snprintf(buf, sizeof(buf), fmt, e.what());
        } catch (...) {
            snprintf(buf, sizeof(buf), fmt, "Unknown Exception");
        }

        ERROR("Exception in cpp-httplib: {}", std::string(buf));

        res.set_content(buf, "text/html");
        res.status = 500;
    });

    std::string listenIp = bindAll ? "0.0.0.0" : browserIp;
    if (!svr.listen(listenIp, browserPort)) {
        CRITICAL("Call of listen failed: ip {}, port {}, Reason: {}", listenIp, browserPort, strerror(errno));
        exit(1);
    }
}

// BrowserApp
BrowserApp::BrowserApp(std::string vdrIp, int vdrPort, std::string transcoderIp, int transcoderPort, std::string browserIp, int browserPort, image_type_enum osdqoi, int zoom_width, int zoom_height, bool use_dirty_recs, std::string static_path, bool bindAll) :
        browserIp(browserIp), browserPort(browserPort),
        transcoderIp(transcoderIp), transcoderPort(transcoderPort),
        vdrIp(vdrIp), vdrPort(vdrPort), osdqoi(osdqoi), zoom_width(zoom_width), zoom_height(zoom_height),
        use_dirty_recs(use_dirty_recs), static_path(static_path), bindAll(bindAll) {

    CefMessageRouterConfig config;
    config.js_query_function = "cefQuery";
    config.js_cancel_function = "cefQueryCancel";
}

CefRefPtr<CefBrowserProcessHandler> BrowserApp::GetBrowserProcessHandler() {
    return this;
}

CefRefPtr<CefRenderProcessHandler> BrowserApp::GetRenderProcessHandler() {
    return this;
}

void BrowserApp::OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line) {
    command_line->AppendSwitchWithValue("autoplay-policy", "no-user-gesture-required");
}

void BrowserApp::OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar) {
    registrar->AddCustomScheme("client", CEF_SCHEME_OPTION_STANDARD);
}

void BrowserApp::OnContextInitialized() {
    LOG_CURRENT_THREAD();

    DEBUG("BrowserApp::OnContextInitialized()");
    CefBrowserSettings browserSettings;
    browserSettings.windowless_frame_rate = 25;

    CefWindowInfo window_info;
    window_info.SetAsWindowless(0);
    window_info.shared_texture_enabled = false;

    CefRefPtr<CefDictionaryValue> extra_info = CefDictionaryValue::Create();
    extra_info->SetString("browserIp", browserIp);
    extra_info->SetInt("browserPort", browserPort);
    extra_info->SetString("transcoderIp", transcoderIp);
    extra_info->SetInt("transcoderPort", transcoderPort);
    extra_info->SetString("vdrIp", vdrIp);
    extra_info->SetInt("vdrPort", vdrPort);
    extra_info->SetInt("LogLevel", logger->level());
    extra_info->SetInt("osdqoi", (int)osdqoi);
    extra_info->SetBool("dirtyRecs", use_dirty_recs);

    CefRefPtr<BrowserClient> client = new BrowserClient(false, zoom_width, zoom_height, vdrIp, vdrPort, transcoderIp, transcoderPort, browserIp, browserPort, osdqoi, use_dirty_recs, static_path);
    currentBrowser = CefBrowserHost::CreateBrowserSync(window_info, client, "", browserSettings, extra_info, nullptr);

    INFO("Start Http Server on {}:{} with static path {}", browserIp, browserPort, static_path);

    std::thread t1(startHttpServer, browserIp, browserPort, vdrIp, vdrPort, transcoderIp, transcoderPort, static_path, bindAll);
    t1.detach();
}

void BrowserApp::OnWebKitInitialized() {
}

void BrowserApp::OnBrowserCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDictionaryValue> extra_info) {
    browserIp = extra_info->GetString("browserIp");
    browserPort = extra_info->GetInt("browserPort");
    transcoderIp = extra_info->GetString("transcoderIp");
    transcoderPort = extra_info->GetInt("transcoderPort");
    vdrIp = extra_info->GetString("vdrIp");
    vdrPort = extra_info->GetInt("vdrPort");
    osdqoi = (image_type_enum)extra_info->GetInt("osdqoi");
    use_dirty_recs = extra_info->GetBool("dirtyRecs");

    int logLevel = extra_info->GetInt("LogLevel");
    logger->set_level(static_cast<spdlog::level::level_enum>(logLevel));
}

void BrowserApp::OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) {
    TRACE("BrowserApp::OnContextCreated");

    // register all native JS functions
    CefRefPtr<CefV8Value> object = context->GetGlobal();
    handler = new V8Handler(browserIp, browserPort, transcoderIp, transcoderPort, vdrIp, vdrPort);

    CefRefPtr<CefV8Value> streamVideo = CefV8Value::CreateFunction("StreamVideo", handler);
    object->SetValue("cefStreamVideo", streamVideo, V8_PROPERTY_ATTRIBUTE_NONE);

    CefRefPtr<CefV8Value> stopVideo = CefV8Value::CreateFunction("StopVideo", handler);
    object->SetValue("cefStopVideo", stopVideo, V8_PROPERTY_ATTRIBUTE_NONE);

    CefRefPtr<CefV8Value> pauseVideo = CefV8Value::CreateFunction("PauseVideo", handler);
    object->SetValue("cefPauseVideo", pauseVideo, V8_PROPERTY_ATTRIBUTE_NONE);

    CefRefPtr<CefV8Value> resumeVideo = CefV8Value::CreateFunction("ResumeVideo", handler);
    object->SetValue("cefResumeVideo", resumeVideo, V8_PROPERTY_ATTRIBUTE_NONE);

    CefRefPtr<CefV8Value> seekVideo = CefV8Value::CreateFunction("SeekVideo", handler);
    object->SetValue("cefSeekVideo", seekVideo, V8_PROPERTY_ATTRIBUTE_NONE);

    CefRefPtr<CefV8Value> redButton = CefV8Value::CreateFunction("RedButton", handler);
    object->SetValue("cefRedButton", redButton, V8_PROPERTY_ATTRIBUTE_NONE);

    CefRefPtr<CefV8Value> loadUrl = CefV8Value::CreateFunction("LoadUrl", handler);
    object->SetValue("cefLoadUrl", loadUrl, V8_PROPERTY_ATTRIBUTE_NONE);

    CefRefPtr<CefV8Value> videoSize = CefV8Value::CreateFunction("VideoSize", handler);
    object->SetValue("cefVideoSize", videoSize, V8_PROPERTY_ATTRIBUTE_NONE);

    CefRefPtr<CefV8Value> videoFull = CefV8Value::CreateFunction("VideoFullscreen", handler);
    object->SetValue("cefVideoFullscreen", videoFull, V8_PROPERTY_ATTRIBUTE_NONE);

    CefRefPtr<CefV8Value> startApp = CefV8Value::CreateFunction("StartApp", handler);
    object->SetValue("cefStartApp", startApp, V8_PROPERTY_ATTRIBUTE_NONE);
}

void BrowserApp::OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) {
    TRACE("BrowserApp::OnContextReleased");
    handler = nullptr;
    svr.stop();
}

bool BrowserApp::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message) {
    TRACE("BrowserApp::OnProcessMessageReceived(Renderer) {}", message->GetName().ToString());

    return false;
}


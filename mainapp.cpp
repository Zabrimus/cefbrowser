#include "mainapp.h"

#include <utility>
#include "browserclient.h"
#include "v8handler.h"
#include "tools.h"
#include "keycodes.h"
#include "database.h"

scoped_refptr<CefBrowser> currentBrowser;

httplib::Server svr;

void startHttpServer(std::string browserIp, int browserPort) {
    auto ret = svr.set_mount_point("/js", "./js");
    if (!ret) {
        // must not happen
        ERROR("http mount point ./js does not exists. Application will not work as desired.");
        return;
    }

    ret = svr.set_mount_point("/css", "./css");
    if (!ret) {
        // must not happen
        ERROR("http mount point ./css does not exists. Application will not work as desired.");
        return;
    }

    // called by VDR
    svr.Post("/LoadUrl", [](const httplib::Request &req, httplib::Response &res) {
        auto url = req.get_param_value("url");
        INFO("Load URL: {}", url);

        if (url.empty()) {
            res.status = 404;
        } else {
            currentBrowser->GetMainFrame()->LoadURL(url);
            res.set_content("ok", "text/plain");
        }
    });

    svr.Post("/RedButton", [](const httplib::Request &req, httplib::Response &res) {
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
                _dynamic.open ("js/_dynamic.js", std::ios_base::trunc);
                _dynamic << "window.HBBTV_POLYFILL_NS = window.HBBTV_POLYFILL_NS || {}; window.HBBTV_POLYFILL_NS.currentChannel = " << channel << std::endl;
                _dynamic.close();

                // load url
                currentBrowser->GetMainFrame()->LoadURL(url);
                res.set_content("ok", "text/plain");
            }
        }
    });

    // called by VDR
    svr.Post("/ProcessKey", [](const httplib::Request &req, httplib::Response &res) {
        auto key = req.get_param_value("key");
        INFO("ProcessKey: {}", key);

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
                frame->ExecuteJavaScript(script, frame->GetURL(), 0);
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
    svr.Post("/ProcessTSPacket", [](const httplib::Request &req, httplib::Response &res) {
        const std::string body = req.body;

        if (body.empty()) {
            res.status = 404;
        } else {
            TRACE("ProcessTSPacket, length {}", body.length());

            if (!vdrRemoteClient->ProcessTSPacket(std::string(body.c_str(), body.length()))) {
                // vdr is not running? stop transcoder
                ERROR("vdr is not running? stop transcoder");
                transcoderRemoteClient->Stop();
            }

            res.set_content("ok", "text/plain");
        }
    });

    // called by VDR
    svr.Post("/InsertHbbtv", [](const httplib::Request &req, httplib::Response &res) {
        const std::string body = req.body;

        if (body.empty()) {
            res.status = 404;
        } else {
            database.insertHbbtv(body);

            res.set_content("ok", "text/plain");
        }
    });

    // called by VDR
    svr.Post("/InsertChannel", [](const httplib::Request &req, httplib::Response &res) {
        const std::string body = req.body;

        if (body.empty()) {
            res.status = 404;
        } else {
            database.insertChannel(body);

            res.set_content("ok", "text/plain");
        }
    });

    svr.listen(browserIp, browserPort);
}

// BrowserApp
BrowserApp::BrowserApp(std::string vdrIp, int vdrPort, std::string transcoderIp, int transcoderPort, std::string browserIp, int browserPort) :
        browserIp(browserIp), browserPort(browserPort),
        transcoderIp(transcoderIp), transcoderPort(transcoderPort),
        vdrIp(vdrIp), vdrPort(vdrPort) {

    CefMessageRouterConfig config;
    config.js_query_function = "cefQuery";
    config.js_cancel_function = "cefQueryCancel";
}

CefRefPtr<CefBrowserProcessHandler> BrowserApp::GetBrowserProcessHandler() {
    DEBUG("BrowserApp::GetBrowserProcessHandler()");

    return this;
}

CefRefPtr<CefRenderProcessHandler> BrowserApp::GetRenderProcessHandler() {
    DEBUG("BrowserApp::GetRenderProcessHandler()");
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
    window_info.shared_texture_enabled = true;

    CefRefPtr<CefDictionaryValue> extra_info = CefDictionaryValue::Create();
    extra_info->SetString("browserIp", browserIp);
    extra_info->SetInt("browserPort", browserPort);
    extra_info->SetString("transcoderIp", transcoderIp);
    extra_info->SetInt("transcoderPort", transcoderPort);
    extra_info->SetString("vdrIp", vdrIp);
    extra_info->SetInt("vdrPort", vdrPort);

    CefRefPtr<BrowserClient> client = new BrowserClient(false, 1280, 720, vdrIp, vdrPort, transcoderIp, transcoderPort, browserIp, browserPort);
    currentBrowser = CefBrowserHost::CreateBrowserSync(window_info, client, "", browserSettings, extra_info, nullptr);

    INFO("Start Http Server on {}:{}", browserIp, browserPort);

    std::thread t1(startHttpServer, browserIp, browserPort);
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
}

void BrowserApp::OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) {
    ERROR("BrowserApp::OnContextCreated");

    ERROR("===> TRANS IP {}", transcoderIp);
    ERROR("===> TRANS PO {}", transcoderPort);

    ERROR("===> VDR IP {}", vdrIp);
    ERROR("===> VDR PO {}", vdrPort);

    ERROR("===> BROW IP {}", browserIp);
    ERROR("===> BROW PO {}", browserPort);

    // register all native JS functions
    CefRefPtr<CefV8Value> object = context->GetGlobal();
    CefRefPtr<CefV8Handler> handler = new V8Handler(browserIp, browserPort, transcoderIp, transcoderPort, vdrIp, vdrPort);

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
}

void BrowserApp::OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) {
    TRACE("BrowserApp::OnContextReleased");
    svr.stop();
}

bool BrowserApp::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message) {
    TRACE("BrowserApp::OnProcessMessageReceived");
    return false;
}
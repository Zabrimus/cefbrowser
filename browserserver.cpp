#include <iostream>
#include "browserserver.h"
#include "VdrClient.h"
#include "mainapp.h"
#include "logger.h"
#include "database.h"
#include "browserclient.h"
#include "keycodes.h"

std::string lastInsertChannel = "";
std::string streamId;
BrowserParameter BrowserServer::bParameterServer;

BrowserServer::~BrowserServer() {
};

void BrowserServer::setBrowserParameter(BrowserParameter bp) {
    bParameterServer = bp;
    streamId = bParameterServer.browserIp + "_" + std::to_string(bParameterServer.browserPort);
};

void BrowserServer::ping() {
}

bool BrowserServer::LoadUrl(const LoadUrlType &input) {
    INFO("Load URL: {}", input.url);

    if (input.url == "about:blank") {
        // special case
        StopVideoType stopType;
        stopType.reason = input.url;
        StopVideo(stopType);
    }

    if (currentBrowser->GetMainFrame() != nullptr) { // Why is it possible, that MainFrame is null?
        currentBrowser->GetMainFrame()->LoadURL(input.url);
    } else {
        ERROR("MainFrame is null");
    }

    return true;
}

bool BrowserServer::RedButton(const RedButtonType &input) {
    INFO("Red Button for channelId: {}", input.channelId);

    std::string url;

    // iterate multiple times.
    for (int i = 0; i < 5; ++i) {
        url = database.getRedButtonUrl(input.channelId);
        if (!url.empty()) {
            break;
        }

        DEBUG("RedButton currently not available. Wait 250ms for a retry.");
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    if (url.empty()) {
        INFO("RedButton for channelId '{}' finally not found. Sent 404.", input.channelId);
        return false;
    } else {
        DEBUG("RedButton URL found: {}", url);

        // write channel information
        std::string channel = database.getChannel(input.channelId);
        std::ofstream _dynamic;
        _dynamic.open (bParameterServer.static_path + "/js/_dynamic.js", std::ios_base::trunc);
        _dynamic << "window.HBBTV_POLYFILL_NS = window.HBBTV_POLYFILL_NS || {}; window.HBBTV_POLYFILL_NS.currentChannel = " << channel << std::endl;
        _dynamic.close();

        CefRefPtr<CefClient> currentClient = currentBrowser->GetHost()->GetClient();
        auto c = dynamic_cast<BrowserClient *>(currentClient.get());

        std::string newUserAgent = database.getUserAgent(input.channelId);
        INFO("Use UserAgent {} for {}", newUserAgent, input.channelId);
        c->ChangeUserAgent(currentBrowser, newUserAgent);
        c->enableProcessing(true);

        // load url
        if (currentBrowser->GetMainFrame() != nullptr) { // Why is it possible, that MainFrame is null?
            currentBrowser->GetMainFrame()->LoadURL(url);
        } else {
            ERROR("MainFrame is null");
            return false;
        }
    }

    return true;
}

bool BrowserServer::ReloadOSD() {
    currentBrowser->GetHost()->Invalidate(PET_VIEW);
    return true;
}

bool BrowserServer::StartApplication(const StartApplicationType &input) {
    INFO("Start Application, channelId {}, appId {}", input.channelId, input.appId);

    std::string channel = database.getChannel(input.channelId);
    std::ofstream _dynamic;
    _dynamic.open (bParameterServer.static_path + "/js/_dynamic.js", std::ios_base::trunc);

    // write channel information and parameters
    _dynamic << "window.HBBTV_POLYFILL_NS = window.HBBTV_POLYFILL_NS || {}; window.HBBTV_POLYFILL_NS.currentChannel = " << channel << std::endl;

    if (input.url.empty()) {
        _dynamic << "window.HBBTV_POLYFILL_NS.paramBody = '';" << std::endl;
    } else {
        _dynamic << "window.HBBTV_POLYFILL_NS.paramBody = " << input.url << std::endl;
    }

    _dynamic.close();

    CefRefPtr<CefClient> currentClient = currentBrowser->GetHost()->GetClient();
    auto c = dynamic_cast<BrowserClient *>(currentClient.get());

    if (!input.appUserAgent.empty()) {
        c->ChangeUserAgent(currentBrowser, input.appUserAgent);
    } else {
        c->ChangeUserAgent(currentBrowser, "");
    }

    std::string url;

    // create application url
    if (input.appId == "MAIN") {
        url = "http://" + bParameterServer.browserIp + ":" + std::to_string(bParameterServer.browserPort) + "/application/main/main.html";
        c->enableProcessing(true);
    } else if (input.appId == "URL") {
        url = input.url;
        c->enableProcessing(false);
    } else if (input.appId == "M3U") {
        url = "http://" + bParameterServer.browserIp + ":" + std::to_string(bParameterServer.browserPort) + "/application/iptv/catalogue/index.html";
        c->enableProcessing(true);
    }

    DEBUG("Load URL: {}", url);

    // sanity check
    if (url.empty()) {
        url = "http://gibbet.nix.da";
    }

    // load url
    currentBrowser->GetMainFrame()->LoadURL(url);

    return true;
}

bool BrowserServer::ProcessKey(const ProcessKeyType &input) {
    if ((input.key.find("VK_BACK") != std::string::npos) ||
        (input.key.find("VK_RED") != std::string::npos) ||
        (input.key.find("VK_GREEN") != std::string::npos) ||
        (input.key.find("VK_YELLOW") != std::string::npos) ||
        (input.key.find("VK_BLUE") != std::string::npos) ||
        (input.key.find("VK_PLAY") != std::string::npos) ||
        (input.key.find("VK_PAUSE") != std::string::npos) ||
        (input.key.find("VK_STOP") != std::string::npos) ||
        (input.key.find("VK_FAST_FWD") != std::string::npos) ||
        (input.key.find("VK_REWIND") != std::string::npos)) {
        // send key event via Javascript
        std::ostringstream stringStream;
        stringStream << "window.cefKeyPress('" << input.key << "');";
        auto script = stringStream.str();
        auto frame = currentBrowser->GetMainFrame();

        if (frame != nullptr) { // Why is it possiböe, that MainFrame is null?
            frame->ExecuteJavaScript(script, frame->GetURL(), 0);
        } else {
            ERROR("MainFrame is null");
            return false;
        }
    } else {
        // send key event via code
        // FIXME: Was ist mit keyCodes, die nicht existieren?
        int windowsKeyCode = keyCodes[input.key];
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

    return true;
}

bool BrowserServer::StreamError(const StreamErrorType &input) {
    ERROR("Transcoder returned a stream error: {}", input.reason);

    std::string script1 = "document.getElementsByTagName('video')[0].currentTime = document.getElementsByTagName('video')[0].duration;";
    currentBrowser->GetMainFrame()->ExecuteJavaScript(script1, currentBrowser->GetMainFrame()->GetURL(), 0);

    std::string script2 = "window.cefKeyPress('VK_BACK');";
    currentBrowser->GetMainFrame()->ExecuteJavaScript(script2, currentBrowser->GetMainFrame()->GetURL(), 0);

    StopVideoType stopType;
    stopType.reason = "stream error";
    StopVideo(stopType);

    return true;
}

bool BrowserServer::InsertHbbtv(const InsertHbbtvType &input) {
    // TRACE("InsertHbbtv: {}", body);
    return database.insertHbbtv(input.hbbtv);
}

bool BrowserServer::InsertChannel(const InsertChannelType &input) {
    // TRACE("InsertChannel: {}", body);

    if (input.channel != lastInsertChannel) {
        // in case of channel switch always stop playing videos
        std::string reason = "ChannelSwitch";

        StopVideoType stopType;
        stopType.reason = "ChannelSwitch";
        StopVideo(stopType);

        lastInsertChannel = input.channel;
    }

    return database.insertChannel(input.channel);
}

bool BrowserServer::StopVideo(const StopVideoType &input) {
    CefRefPtr<CefClient> currentClient = currentBrowser->GetHost()->GetClient();
    auto c = dynamic_cast<BrowserClient *>(currentClient.get());
    c->getCurrentTranscoderClient()->Stop(streamId, input.reason);

    return true;
}

CefBrowserIf *BrowserCloneFactory::getHandler(const TConnectionInfo &connInfo) {
    std::shared_ptr<TSocket> sock = std::dynamic_pointer_cast<TSocket>(connInfo.transport);

    /*
    std::cout << "Incoming connection\n";
    std::cout << "\tSocketInfo: "  << sock->getSocketInfo() << "\n";
    std::cout << "\tPeerHost: "    << sock->getPeerHost() << "\n";
    std::cout << "\tPeerAddress: " << sock->getPeerAddress() << "\n";
    std::cout << "\tPeerPort: "    << sock->getPeerPort() << "\n";
    */

    return new BrowserServer();

}

void BrowserCloneFactory::releaseHandler(CommonServiceIf *handler) {
    delete handler;
}

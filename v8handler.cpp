#include "v8handler.h"
#include "logger.h"

V8Handler::V8Handler(std::string bIp, int bPort, std::string tIp, int tPort, std::string vdrIp, int vdrPort)
        : browserIp(bIp), browserPort(bPort), transcoderIp(tIp), transcoderPort(tPort), vdrIp(vdrIp), vdrPort(vdrPort)
{
    transcoderRemoteClient = new TranscoderRemoteClient(tIp, tPort, bIp, bPort);
    vdrRemoteClient = new VdrRemoteClient(vdrIp, vdrPort);
}

V8Handler::~V8Handler() {
    delete transcoderRemoteClient;
    delete vdrRemoteClient;
}

void V8Handler::sendMessageToBrowser(std::string message) {
    sendMessageToBrowser(message, "");
}

void V8Handler::sendMessageToBrowser(std::string message, std::string parameter) {
    CefRefPtr<CefV8Context> ctx = CefV8Context::GetCurrentContext();
    CefRefPtr<CefBrowser> browser = ctx->GetBrowser();

    CefRefPtr<CefProcessMessage> msg= CefProcessMessage::Create(message);
    CefRefPtr<CefListValue> args = msg->GetArgumentList();
    if (!parameter.empty()) {
        args->SetString(0, parameter);
    }

    browser->GetMainFrame()->SendProcessMessage(PID_BROWSER, msg);
}

void V8Handler::sendMessageToBrowser(std::string message, std::vector<std::string>& parameter) {
    CefRefPtr<CefV8Context> ctx = CefV8Context::GetCurrentContext();
    CefRefPtr<CefBrowser> browser = ctx->GetBrowser();

    CefRefPtr<CefProcessMessage> msg= CefProcessMessage::Create(message);
    CefRefPtr<CefListValue> args = msg->GetArgumentList();

    int idx = 0;
    for (const auto &item: parameter) {
        args->SetString(idx, item);
        idx++;
    }

    browser->GetMainFrame()->SendProcessMessage(PID_BROWSER, msg);
}


bool V8Handler::Execute(const CefString &name, CefRefPtr<CefV8Value> object, const CefV8ValueList &arguments, CefRefPtr<CefV8Value> &retval, CefString &exception) {
    DEBUG("V8Handler::Execute: {}", name.ToString());

    if (name == "StreamVideo") {
        if (!arguments.empty()) {
            auto urlParam = arguments.at(0);
            auto url = urlParam.get()->GetStringValue();

            vdrRemoteClient->StartVideo();

            if (!transcoderRemoteClient->StreamUrl(url)) {
                // transcoder not available
                ERROR("Unable to send request to transcoder");
                return false;
            }

            sendMessageToBrowser("StreamVideo", url);
        }

        retval = CefV8Value::CreateString("http://"+ transcoderIp + ":" + std::to_string(transcoderPort) + "/movie/transparent-video-" + browserIp + "_" + std::to_string(browserPort) + ".webm");
        return true;
    } else if (name == "StopVideo") {
        vdrRemoteClient->StopVideo();
        sendMessageToBrowser("StopVideo");

        retval = CefV8Value::CreateBool(true);
        return true;
    } else if (name == "PauseVideo") {
        sendMessageToBrowser("PauseVideo");

        retval = CefV8Value::CreateBool(true);
        return true;
    } else if (name == "ResumeVideo") {
        sendMessageToBrowser("ResumeVideo");

        retval = CefV8Value::CreateBool(true);
        return true;
    } else if (name == "SeekVideo") {
        DEBUG("V8Handler::Execute SeekVideo");

        if (!arguments.empty()) {
            auto param = arguments.at(0);
            auto position = param.get()->GetStringValue().ToString();

            DEBUG("V8Handler::Execute SeekVideo Argument Pos {}", position);

            sendMessageToBrowser("SeekVideo", position);
        }

        retval = CefV8Value::CreateBool(true);
        return true;
    } else if (name == "RedButton") {
        DEBUG("V8Handler::Execute RedButton");

        if (!arguments.empty()) {
            auto param = arguments.at(0);
            auto channelId = param.get()->GetStringValue().ToString();

            DEBUG("V8Handler::Execute RedButton Argument ChannelId {}", channelId);

            sendMessageToBrowser("RedButton", channelId);
        }

        retval = CefV8Value::CreateBool(true);
        return true;
    } else if (name == "LoadUrl") {
        DEBUG("V8Handler::Execute LoadUrl");

        if (!arguments.empty()) {
            auto param = arguments.at(0);
            auto url = param.get()->GetStringValue().ToString();

            DEBUG("V8Handler::Execute LoadUrl {}", url);

            sendMessageToBrowser("LoadUrl", url);
        }

        retval = CefV8Value::CreateBool(true);
        return true;
    }  else if (name == "VideoSize") {
        DEBUG("V8Handler::Execute VideoSize");

        if (!arguments.empty()) {
            const auto x = arguments.at(0)->GetIntValue();
            const auto y = arguments.at(1)->GetIntValue();
            const auto w = arguments.at(2)->GetIntValue();
            const auto h = arguments.at(3)->GetIntValue();

            std::vector<std::string> params;
            params.push_back(std::to_string(x));
            params.push_back(std::to_string(y));
            params.push_back(std::to_string(w));
            params.push_back(std::to_string(h));

            sendMessageToBrowser("VideoSize", params);

            vdrRemoteClient->VideoSize(x, y, w, h);
        }

        retval = CefV8Value::CreateBool(true);
        return true;
    } else if (name == "VideoFullscreen") {
        DEBUG("V8Handler::Execute VideoFullscreen");

        sendMessageToBrowser("VideoFullscreen");
        vdrRemoteClient->VideoFullscreen();

        retval = CefV8Value::CreateBool(true);
        return true;
    } else if (name == "StartApp") {
        DEBUG("V8Handler::Execute StartApp");

        if (!arguments.empty()) {
            const auto channelId = arguments.at(0)->GetStringValue();
            const auto appId = arguments.at(1)->GetStringValue();
            const auto args = arguments.at(2)->GetStringValue();

            std::vector<std::string> params;
            params.push_back(channelId);
            params.push_back(appId);
            params.push_back(args);

            sendMessageToBrowser("StartApp", params);
        }

        retval = CefV8Value::CreateBool(true);
        return true;
    } else {
        ERROR("Javascript function {} is not implemented.", name.ToString());
    }

    // Function does not exist.
    return false;
}

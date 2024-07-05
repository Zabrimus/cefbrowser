#pragma once

#include "cef_includes.h"
#include "v8handler.h"
#include "tools.h"
#include "httplib.h"

class BrowserApp : public CefApp,
                   public CefBrowserProcessHandler,
                   public CefRenderProcessHandler {
public:
    BrowserApp(std::string vdrIp, int vdrPort, std::string transcoderIp, int transcoderPort, std::string browserIp, int browserPort, image_type_enum osdqoi, int zoom_width, int zoom_height, bool use_dirty_recs, std::string static_path, bool bindAll);

    // CefApp
    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override;
    CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override;

    void OnBeforeCommandLineProcessing(const CefString &process_type, CefRefPtr<CefCommandLine> command_line) override;
    void OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar) override;

    // CefBrowserProcessHandler
    void OnContextInitialized() override;

    // CefRenderProcessHandler methods:
    void OnWebKitInitialized() override;
    void OnBrowserCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDictionaryValue> extra_info) override;
    void OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) override;
    void OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) override;
    bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message) override;

private:
    std::string browserIp;
    int browserPort;

    std::string transcoderIp;
    int transcoderPort;

    std::string vdrIp;
    int vdrPort;

    image_type_enum osdqoi;
    int zoom_width;
    int zoom_height;
    bool use_dirty_recs;
    std::string static_path;

    bool bindAll;

    CefRefPtr<V8Handler> handler;

private:
    IMPLEMENT_REFCOUNTING(BrowserApp);
    DISALLOW_COPY_AND_ASSIGN(BrowserApp);
};

extern scoped_refptr<CefBrowser> currentBrowser;

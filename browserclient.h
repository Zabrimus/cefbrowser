#pragma once

#include "cef_includes.h"
#include "requestresponse.h"
#include "logger.h"
#include "tools.h"
#include "transcoderremoteclient.h"
#include "vdrremoteclient.h"
#include "sharedmemory.h"

class BrowserClient : public CefClient,
                      public CefRenderHandler,
                      public CefLifeSpanHandler,
                      public CefRequestHandler,
                      public CefDisplayHandler {

public:
    explicit BrowserClient(bool fullscreen, int width, int height,
                           std::string vdrIp, int vdrPort,
                           std::string transcoderIp, int transcoderPort,
                           std::string browserIp, int browserPort,
                           image_type_enum osdqoi,
                           bool use_dirty_recs);

    ~BrowserClient() override;

    // RenderHandler
    CefRefPtr<CefRenderHandler> GetRenderHandler() override {
        return this;
    };

    // CefLifeSpanHandler
    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override {
        return this;
    }

    // CefRequestHandler
    CefRefPtr<CefRequestHandler> GetRequestHandler() override {
        return this;
    }

    // CefDisplayHandler
    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override {
        return this;
    }

    // CefClient
    bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message) override;

    // CefResourceRequestHandler
    CefRefPtr<CefResourceRequestHandler> GetResourceRequestHandler(
            CefRefPtr<CefBrowser> browser,
            CefRefPtr<CefFrame> frame,
            CefRefPtr<CefRequest> request,
            bool is_navigation,
            bool is_download,
            const CefString &request_initiator,
            bool &disable_default_handling) override;

    // RenderHandler
    void setRenderSize(int width, int height);

    void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) override;

    void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer,
                 int width, int height) override;

    // LifeSpanHandler
    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
    void OnBeforeClose(CefRefPtr< CefBrowser > browser) override;

    // RequestHandler
    bool OnBeforeBrowse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool user_gesture, bool is_redirect) override;
    void OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser, TerminationStatus status) override;

    // CefDisplayHandler
    void OnStatusMessage(CefRefPtr<CefBrowser> browser, const CefString& value) override;
    bool OnConsoleMessage(CefRefPtr<CefBrowser> browser, cef_log_severity_t level, const CefString& message, const CefString& source, int line) override;

private:
    void loadUrl(CefRefPtr<CefBrowser> browser, const std::string& url);

private:
    int renderWidth;
    int renderHeight;
    bool fullscreen;

    std::string vdrIp;
    int vdrPort;

    std::string transcoderIp;
    int transcoderPort;

    std::string browserIp;
    int browserPort;

    image_type_enum osdqoi;
    bool use_dirty_recs;

    int videoX, videoY, videoW, videoH;
    bool videoIsFullscreen;

    VdrRemoteClient* vdrRemoteClient;
    TranscoderRemoteClient *transcoderRemoteClient;

    SharedMemory sharedMemory;

private:
    IMPLEMENT_REFCOUNTING(BrowserClient);
};

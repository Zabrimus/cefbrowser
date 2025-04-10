#pragma once

#include <string>
#include "cef_includes.h"
#include "logger.h"
#include "pagemodifier.h"

class StaticHandler : public CefResourceRequestHandler,
                      public CefResourceHandler {
public:
    StaticHandler(std::string static_path, std::string url);

    // CefResourceRequestHandler
    CefRefPtr<CefResourceHandler> GetResourceHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                                     CefRefPtr<CefRequest> request) override {
        return this;
    }

    CefResourceRequestHandler::ReturnValue
    OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request,
                         CefRefPtr<CefCallback> callback) override;

    void OnResourceLoadComplete(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request,
                                CefRefPtr<CefResponse> response, URLRequestStatus status,
                                int64_t received_content_length) override;

    // CefResourceHandler
    bool Open(CefRefPtr<CefRequest> request, bool &handle_request, CefRefPtr<CefCallback> callback) override;

    bool Read(void *data_out, int bytes_to_read, int &bytes_read, CefRefPtr<CefResourceReadCallback> callback) override;

    void GetResponseHeaders(CefRefPtr<CefResponse> response, int64_t &response_length, CefString &redirectUrl) override;

    void Cancel() override;

private:
    // cpr::Response cprResponse;
    std::string requestUrl;
    std::string staticPath;
    std::string content;
    std::string mimeType;
    size_t offset;

private:
    std::string guessMimeType(std::string& name);
    static PageModifier modifier;

private:
    IMPLEMENT_REFCOUNTING(StaticHandler);
};

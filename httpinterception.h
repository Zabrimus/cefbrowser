#pragma once

#include <string>
#include <utility>
#include "cef_includes.h"
#include "logger.h"
#include "pagemodifier.h"
#include "lift.h"

class HttpRequestClient;

class HttpInterception : public CefResourceRequestHandler,
                         public CefResourceHandler,
                         public LiftCallback {
public:
    HttpInterception(std::string staticPath, bool blockThis);

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

    // LiftCallback
    auto on_lift_complete(lift::request_ptr request_ptr, lift::response response) -> void override;

    static PageModifier modifier;

private:
    bool blockThis;

    // will be filled in the lift callback (on_lift_complete)
    CefResponse::HeaderMap responseHeaderMap;
    int64_t download_total = 0;
    size_t offset = 0;
    std::string download_data;
    std::string mime_type;
    lift::http::status_code status_code;

    CefRefPtr<CefCallback> callback;

private:
    IMPLEMENT_REFCOUNTING(HttpInterception);
};

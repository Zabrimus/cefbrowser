#pragma once

#include <string>
#include <utility>
#include "cef_includes.h"
#include "cpr/connection_pool.h"
#include "cpr/response.h"

class BaseInterception : public CefResourceRequestHandler,
                         public CefResourceHandler {
public:
    BaseInterception(std::string interceptorName, const cpr::ConnectionPool& pool) : interceptorName(std::move(interceptorName)), pool(pool) {
    }

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

protected:
    virtual void modifyResponse();

    void logResponseHeaders() const;

protected:
    cpr::Response cprResponse;
    CefResponse::HeaderMap responseHeaderMap;

    std::string mimeType;
    size_t offset = 0;

    CefRefPtr<CefCallback> callback;

private:
    std::string interceptorName;
    cpr::ConnectionPool pool;

    BaseInterception() = default;

    IMPLEMENT_REFCOUNTING(BaseInterception);
};

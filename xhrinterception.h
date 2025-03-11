#pragma once

#include <string>
#include "cef_includes.h"
#include "logger.h"

class XhrRequestClient;

class XhrInterception : public CefResourceRequestHandler,
                       public CefResourceHandler {
public:
    XhrInterception();

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
    CefRefPtr<XhrRequestClient> client;
    CefRefPtr<CefURLRequest> url_request;

private:
    IMPLEMENT_REFCOUNTING(XhrInterception);
};


class XhrRequestClient : public CefURLRequestClient {
    friend XhrInterception;

public:
    explicit XhrRequestClient(CefRefPtr<CefCallback>&);

    void OnRequestComplete(CefRefPtr<CefURLRequest> request) override;

    void OnUploadProgress(CefRefPtr<CefURLRequest> request, int64_t current, int64_t total) override;

    void OnDownloadProgress(CefRefPtr<CefURLRequest> request, int64_t current, int64_t total) override;

    void OnDownloadData(CefRefPtr<CefURLRequest> request, const void *data, size_t data_length) override;

    bool GetAuthCredentials(bool isProxy, const CefString &host, int port, const CefString &realm,
                            const CefString &scheme, CefRefPtr<CefAuthCallback> callback) override;

private:
    size_t upload_total;
    size_t download_total;
    size_t offset;
    std::string download_data;

    CefResponse::HeaderMap headerMap;

    CefRefPtr<CefCallback> callback;

private:
    IMPLEMENT_REFCOUNTING(XhrRequestClient);
};

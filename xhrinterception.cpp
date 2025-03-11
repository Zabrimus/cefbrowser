#include <cstdio>
#include "tools.h"
#include "httplib.h"
#include "xhrinterception.h"
#include "json.hpp"

// CefResourceRequestHandler
CefResourceRequestHandler::ReturnValue XhrInterception::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) {
    TRACE("XhrRequestResponse::OnBeforeResourceLoad: {}", request->GetURL().ToString());

    client = new XhrRequestClient(callback);
    url_request = CefURLRequest::Create(request, client.get(), nullptr);

    return RV_CONTINUE_ASYNC;
}

XhrInterception::XhrInterception() {
}

bool XhrInterception::Open(CefRefPtr<CefRequest> request, bool& handle_request, CefRefPtr<CefCallback> callback) {
    TRACE("XhrInterception::Open: {}", request->GetURL().ToString());

    handle_request = true;
    return true;
}

bool XhrInterception::Read(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefResourceReadCallback> callback) {
    DEBUG("XhrInterception::Read: {}, Size {}, Offset {}", bytes_to_read, client->download_data.length(), client->offset);

    size_t size = client->download_data.length();
    if (client->offset < size) {
        int transfer_size = std::min(bytes_to_read, static_cast<int>(size - client->offset));
        memcpy(data_out, client->download_data.c_str() + client->offset, transfer_size);
        client->offset += transfer_size;

        bytes_read = transfer_size;
        return true;
    }

    return false;
}

void XhrInterception::GetResponseHeaders(CefRefPtr<CefResponse> response, int64_t &response_length, CefString &redirectUrl) {
    DEBUG("XhrInterception::XhrInterception: Error1={}, Error2={}, Status1={}, Status2={}, StatusText={}",
            (int)url_request->GetResponse()->GetError(), (int)url_request->GetRequestError(),
            (int)url_request->GetResponse()->GetStatus(), (int)url_request->GetRequestStatus(),
            url_request->GetResponse()->GetStatusText().ToString() );

    if (url_request->GetResponse()->GetStatus() != 200) {
        response->SetStatus(url_request->GetResponse()->GetStatus());
        response->SetStatusText(url_request->GetResponse()->GetStatusText());
        response_length = 0;

        return;
    }

    CefResponse::HeaderMap responseHeader;
    url_request->GetResponse()->GetHeaderMap(responseHeader);
    for (auto itr = responseHeader.begin(); itr != responseHeader.end(); ++itr) {
        TRACE("ResponseHeader: {} -> {}", itr->first.ToString(), itr->second.ToString());
    }

    CefRequest::HeaderMap requestHeader;
    url_request->GetRequest()->GetHeaderMap(requestHeader);
    for (auto itr = requestHeader.begin(); itr != requestHeader.end(); ++itr) {
        TRACE("RequestHeader: {} -> {}", itr->first.ToString(), itr->second.ToString());
    }

    response->SetStatus(200);
    response->SetStatusText("OK");
    response->SetHeaderMap(responseHeader);

    response_length = client->download_total;
}

void XhrInterception::Cancel() {
}

void XhrInterception::OnResourceLoadComplete(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                             CefRefPtr<CefRequest> request, CefRefPtr<CefResponse> response,
                                             CefResourceRequestHandler::URLRequestStatus status,
                                             int64_t received_content_length) {
    DEBUG("XhrInterception::OnResourceLoadComplete: {} -> {}", (int)status, received_content_length);

    CefResourceRequestHandler::OnResourceLoadComplete(browser, frame, request, response, status, received_content_length);
}

XhrRequestClient::XhrRequestClient(CefRefPtr<CefCallback>& resourceCallback) : upload_total(0), download_total(0), offset(0), callback(resourceCallback) {
}

void XhrRequestClient::OnRequestComplete(CefRefPtr<CefURLRequest> request) {
    TRACE("XhrRequestClient::OnRequestComplete:  {}, {}, {}", (int)request->GetRequestStatus(), (int)request->GetRequestError(), request->GetResponse()->GetMimeType().ToString());

    request->GetResponse()->GetHeaderMap(headerMap);

    // hbbtv.zdf.de
    if (request->GetRequest()->GetURL().ToString().find("hbbtv.zdf.de") != std::string::npos) {
        auto dataJson = nlohmann::json::parse(download_data);

        size_t count;

        // neue Version
        if (dataJson.find("data") != dataJson.end()) {
            dataJson["data"].erase("ageControl");
        }

        // alte Version
        if (dataJson.find("fsk") != dataJson.end()) {
            dataJson["fsk"].erase("age");
        }

        download_data = dataJson.dump();
    }

    callback->Continue();
}

void XhrRequestClient::OnUploadProgress(CefRefPtr<CefURLRequest> request, int64_t current, int64_t total) {
    upload_total = total;
}

void XhrRequestClient::OnDownloadProgress(CefRefPtr<CefURLRequest> request, int64_t current, int64_t total) {
    download_total = total;
}

void XhrRequestClient::OnDownloadData(CefRefPtr<CefURLRequest> request, const void *data, size_t data_length) {
    DEBUG("XhrRequestClient::OnDownloadData: {}: {} -> {}", request->GetRequest()->GetURL().ToString(), data_length, download_data.length());

    std::string downloadChunk = std::string(static_cast<const char*>(data), data_length);
    download_data += downloadChunk;
}

bool XhrRequestClient::GetAuthCredentials(bool isProxy, const CefString &host, int port, const CefString &realm,
                                       const CefString &scheme, CefRefPtr<CefAuthCallback> callback) {
    TRACE("XhrRequestClient::GetAuthCredentials: {}, {}", host.ToString(), port);
    return false;
}

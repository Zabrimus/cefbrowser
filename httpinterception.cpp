#include <cstdio>
#include <map>
#include <utility>
#include "tools.h"
#include "httpinterception.h"
#include "json.hpp"

PageModifier HttpInterception::modifier;

// CefResourceRequestHandler
CefResourceRequestHandler::ReturnValue HttpInterception::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) {
    TRACE("HttpInterception::OnBeforeResourceLoad: {}", request->GetURL().ToString());

    if (blockThis) {
        return RV_CANCEL;
    }

    client = new HttpRequestClient(callback);
    url_request = CefURLRequest::Create(request, client.get(), nullptr);

    return RV_CONTINUE_ASYNC;
}

HttpInterception::HttpInterception(std::string staticPath, bool blockThis) {
    modifier.init(std::move(staticPath));
    this->blockThis = blockThis;
}

bool HttpInterception::Open(CefRefPtr<CefRequest> request, bool& handle_request, CefRefPtr<CefCallback> callback) {
    TRACE("HttpInterception::Open: {}", request->GetURL().ToString());

    handle_request = true;
    return true;
}

bool HttpInterception::Read(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefResourceReadCallback> callback) {
    TRACE("HttpInterception::Read: {}, Size {}, Offset {}", bytes_to_read, client->download_data.length(), client->offset);

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

void HttpInterception::GetResponseHeaders(CefRefPtr<CefResponse> response, int64_t &response_length, CefString &redirectUrl) {
    DEBUG("HttpInterception::HttpInterception: Error1={}, Error2={}, Status1={}, Status2={}, StatusText={}",
            (int)url_request->GetResponse()->GetError(), (int)url_request->GetRequestError(),
            (int)url_request->GetResponse()->GetStatus(), (int)url_request->GetRequestStatus(),
            url_request->GetResponse()->GetStatusText().ToString() );

    CefRequest::HeaderMap requestHeader;
    url_request->GetRequest()->GetHeaderMap(requestHeader);
    for (auto itr = requestHeader.begin(); itr != requestHeader.end(); ++itr) {
        TRACE("RequestHeader: {} -> {}", itr->first.ToString(), itr->second.ToString());
    }

    // find content-type
    std::string contentType;

    CefResponse::HeaderMap responseHeader;
    url_request->GetResponse()->GetHeaderMap(responseHeader);
    for (auto itr = responseHeader.begin(); itr != responseHeader.end(); ++itr) {
        TRACE("ResponseHeader: {} -> {}", itr->first.ToString(), itr->second.ToString());

        auto pos = strcasestr(itr->first.ToString().c_str(), "content-type");
        if(pos != nullptr) {
            contentType = itr->second.ToString();
            responseHeader.erase(itr->first.ToString());
            TRACE("2.Received Content-Type: {} -> {}", contentType, contentType.empty());
        }
    }

    TRACE("Received Content-Type: {} -> {}", contentType, contentType.empty());

    if (!contentType.empty()) {
        if (contentType.find("text/html") != std::string::npos) {
            response->SetMimeType("text/html");
            responseHeader.insert(std::make_pair("Content-Type", "text/html"));
        } else if (contentType.find("application/vnd.hbbtv.xhtml+xml") != std::string::npos) {
            response->SetMimeType("application/xhtml+xml");
            responseHeader.insert(std::make_pair("Content-Type", "application/xhtml+xml;charset=UTF-8"));
        } else {
            // default
            response->SetMimeType("application/xhtml+xml");
            responseHeader.insert(std::make_pair("Content-Type", "application/xhtml+xml;charset=UTF-8"));
        }
    } else {
        // default
        response->SetMimeType("application/xhtml+xml");
        responseHeader.insert(std::make_pair("Content-Type", "application/xhtml+xml;charset=UTF-8"));
    }

    response->SetHeaderMap(responseHeader);

    response_length = client->download_total;

    TRACE("RequestHeader: response_length {}", response_length);
}

void HttpInterception::Cancel() {
}

void HttpInterception::OnResourceLoadComplete(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                             CefRefPtr<CefRequest> request, CefRefPtr<CefResponse> response,
                                             CefResourceRequestHandler::URLRequestStatus status,
                                             int64_t received_content_length) {
    DEBUG("HttpInterception::OnResourceLoadComplete: {} -> {}", (int)status, received_content_length);

    CefResourceRequestHandler::OnResourceLoadComplete(browser, frame, request, response, status, received_content_length);
}

HttpRequestClient::HttpRequestClient(CefRefPtr<CefCallback>& resourceCallback) : download_total(0), offset(0), download_data(""), callback(resourceCallback) {
}

void HttpRequestClient::OnRequestComplete(CefRefPtr<CefURLRequest> request) {
    TRACE("HttpRequestClient::OnRequestComplete:  {}, {}, {}", (int) request->GetRequestStatus(),
          (int) request->GetRequestError(), request->GetResponse()->GetMimeType().ToString());

    // TODO: Seite ändern, falls erforderlich
    download_data = HttpInterception::modifier.injectAll(download_data);
    callback->Continue();
}

void HttpRequestClient::OnDownloadProgress(CefRefPtr<CefURLRequest> request, int64_t current, int64_t total) {
    TRACE("HttpRequestClient::OnDownloadProgress: current({}), total({})", current, total);
    download_total = total;
}

void HttpRequestClient::OnDownloadData(CefRefPtr<CefURLRequest> request, const void *data, size_t data_length) {
    TRACE("HttpRequestClient::OnDownloadData: {}: {} -> {}", request->GetRequest()->GetURL().ToString(), data_length, download_data.length());

    std::string downloadChunk = std::string(static_cast<const char*>(data), data_length);
    download_data += downloadChunk;
}

bool HttpRequestClient::GetAuthCredentials(bool isProxy, const CefString &host, int port, const CefString &realm,
                                       const CefString &scheme, CefRefPtr<CefAuthCallback> callback) {
    TRACE("HttpRequestClient::GetAuthCredentials: {}, {}", host.ToString(), port);
    return false;
}
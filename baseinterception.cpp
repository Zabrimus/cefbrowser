#include "baseinterception.h"
#include "json.hpp"
#include "cpr/api.h"
#include "cpr/response.h"
#include "logger.h"

// CefResourceRequestHandler
CefResourceRequestHandler::ReturnValue BaseInterception::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) {
    TRACE("{}::OnBeforeResourceLoad: {}", interceptorName, request->GetURL().ToString());

    this->callback = callback;
    requestUrl = request->GetURL().ToString();

    cpr::Header header;

    CefRequest::HeaderMap headerMap;
    request->GetHeaderMap(headerMap);
    header.insert(headerMap.begin(), headerMap.end());

    if (request->GetMethod() == "GET") {
        cprResponse = cpr::Get(cpr::Url{request->GetURL().ToString()}, header, pool);
    } else if (request->GetMethod() == "POST") {
        cprResponse = cpr::Post(cpr::Url{request->GetURL().ToString()}, header, pool);
    } else {
        // not implemented
        ERROR("HTTP method {} not implemented", request->GetMethod().ToString());
        return RV_CANCEL;
    }

    logResponseHeaders();
    modifyResponse();

    return RV_CONTINUE;
}

bool BaseInterception::Open(CefRefPtr<CefRequest> request, bool& handle_request, CefRefPtr<CefCallback> _callback) {
    TRACE("{}::Open: {}", interceptorName, request->GetURL().ToString());

    handle_request = true;
    return true;
}

bool BaseInterception::Read(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefResourceReadCallback> _callback) {
    // DEBUG("{}::Read: {}, Size {}, Offset {}", interceptorName, bytes_to_read, cprResponse.text.length(), offset);

    size_t size = cprResponse.text.length();
    if (offset < size) {
        int transfer_size = std::min(bytes_to_read, static_cast<int>(size - offset));
        memcpy(data_out, cprResponse.text.c_str() + offset, transfer_size);
        offset += transfer_size;

        bytes_read = transfer_size;
        return true;
    }

    return false;
}

void BaseInterception::GetResponseHeaders(CefRefPtr<CefResponse> response, int64_t &response_length, CefString &redirectUrl) {
    DEBUG("{}::GetResponseHeaders: StatusCode: {}, StatusText: {}", interceptorName, cprResponse.status_code, cprResponse.status_line);

    // libcurl follows redirects itself, so the body can come from a different
    // URL than the one CEF asked for. Serving it under the original URL leaves
    // the document on the original origin, and every request the page then makes
    // that is itself redirected - http to https, for these broadcasters - counts
    // as cross origin and is dropped by CORS, taking the application's own
    // scripts and data with it. Point CEF at the final URL and let it ask again
    // from there, so document and sub resources share one origin.
    const std::string &finalUrl = cprResponse.url.str();
    if (!finalUrl.empty() && finalUrl != requestUrl) {
        redirectUrl = finalUrl;
        return;
    }

    responseHeaderMap.insert(cprResponse.header.begin(), cprResponse.header.end());

    response->SetHeaderMap(responseHeaderMap);
    response_length = cprResponse.downloaded_bytes;

    // The status was never passed on, so every intercepted response looked like
    // a success to the page no matter what the server said.
    if (cprResponse.status_code > 0) {
        response->SetStatus(cprResponse.status_code);
    }

    if (!mimeType.empty()) {
        response->SetMimeType(mimeType);
    }
}

void BaseInterception::Cancel() {
}

void BaseInterception::OnResourceLoadComplete(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                             CefRefPtr<CefRequest> request, CefRefPtr<CefResponse> response,
                                             CefResourceRequestHandler::URLRequestStatus status,
                                             int64_t received_content_length) {
    DEBUG("{}::OnResourceLoadComplete: {} -> {}", interceptorName, cprResponse.status_code, cprResponse.downloaded_bytes);
}

void BaseInterception::logResponseHeaders() const {
    if (logger->isTraceEnabled()) {
        for (auto header: cprResponse.header) {
            TRACE("   {}:{}", header.first, header.second);
        }
    }
}

void BaseInterception::modifyResponse() {
    // default implementation does nothing
}

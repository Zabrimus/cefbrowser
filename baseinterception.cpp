#include "baseinterception.h"
#include "json.hpp"
#include "cpr/api.h"
#include "cpr/response.h"
#include "logger.h"

// CefResourceRequestHandler
CefResourceRequestHandler::ReturnValue BaseInterception::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) {
    TRACE("{}::OnBeforeResourceLoad: {}", interceptorName, request->GetURL().ToString());

    this->callback = callback;

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

    responseHeaderMap.insert(cprResponse.header.begin(), cprResponse.header.end());

    response->SetHeaderMap(responseHeaderMap);
    response_length = cprResponse.downloaded_bytes;

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

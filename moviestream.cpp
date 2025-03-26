#include <cstdio>
#include "tools.h"
#include "moviestream.h"
#include "json.hpp"
#include "TranscoderClient.h"

// CefResourceRequestHandler
CefResourceRequestHandler::ReturnValue MovieStream::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) {
    TRACE("MovieStream::OnBeforeResourceLoad: {}", request->GetURL().ToString());

    return RV_CONTINUE;
}

MovieStream::MovieStream(TranscoderClient *client) : offset(0), download_total(0), transcoderClient(client) {
}

bool MovieStream::Open(CefRefPtr<CefRequest> request, bool& handle_request, CefRefPtr<CefCallback> callback) {
    TRACE("MovieStream::Open: {}", request->GetURL().ToString());

    VideoType input;
    input.filename = request->GetURL().ToString().c_str() + strlen("http://localhost/movie/");
    transcoderClient->GetVideo(download_data, input);
    download_total = download_data.length();

    callback->Continue();

    handle_request = true;
    return true;
}

bool MovieStream::Read(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefResourceReadCallback> callback) {
    // TRACE("MovieStream::Read: {}, Size {}, Offset {}", bytes_to_read, download_data.length(), offset);

    size_t size = download_data.length();
    if (offset < size) {
        int transfer_size = std::min(bytes_to_read, static_cast<int>(size - offset));
        memcpy(data_out, download_data.c_str() + offset, transfer_size);
        offset += transfer_size;

        bytes_read = transfer_size;
        return true;
    }

    return false;
}

void MovieStream::GetResponseHeaders(CefRefPtr<CefResponse> response, int64_t &response_length, CefString &redirectUrl) {
    // TRACE("MovieStream::GetResponseHeaders");

    CefResponse::HeaderMap responseHeader;

    response->SetStatus(206);
    response->SetStatusText("Partial Content");
    response->SetMimeType("video/webm");

    responseHeader.insert(std::make_pair("Content-Type", "video/webm"));
    responseHeader.insert(std::make_pair("Content-Range", "bytes 0-" + std::to_string(download_total-1) + "/" + std::to_string(download_total)));
    responseHeader.insert(std::make_pair("Content-Length", std::to_string(download_total)));

    response->SetHeaderMap(responseHeader);

    response_length = download_total;
}

void MovieStream::Cancel() {
}

void MovieStream::OnResourceLoadComplete(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                             CefRefPtr<CefRequest> request, CefRefPtr<CefResponse> response,
                                             CefResourceRequestHandler::URLRequestStatus status,
                                             int64_t received_content_length) {
    // TRACE("MovieStream::OnResourceLoadComplete: {} -> {}", (int)status, received_content_length);

    CefResourceRequestHandler::OnResourceLoadComplete(browser, frame, request, response, status, received_content_length);
}
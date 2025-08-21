#include <cstdio>
#include "tools.h"
#include "jsinterception.h"
#include "json.hpp"

// CefResourceRequestHandler
CefResourceRequestHandler::ReturnValue JSInterception::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) {
    TRACE("JSInterception::OnBeforeResourceLoad: {}", request->GetURL().ToString());

    this->callback = callback;

    // async version
    LiftUtil::getInstance().get_lift_url(request, 30 * 1000, this);

    // sync version
    // LiftUtil::getInstance().get_lift_url_sync(request, 30 * 1000, this);

    return RV_CONTINUE_ASYNC;
}

JSInterception::JSInterception() {
}

bool JSInterception::Open(CefRefPtr<CefRequest> request, bool& handle_request, CefRefPtr<CefCallback> callback) {
    TRACE("JSInterception::Open: {}", request->GetURL().ToString());

    handle_request = true;
    return true;
}

bool JSInterception::Read(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefResourceReadCallback> callback) {
    // DEBUG("JSInterception::Read: {}, Size {}, Offset {}", bytes_to_read, client->download_data.length(), client->offset);

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

void JSInterception::GetResponseHeaders(CefRefPtr<CefResponse> response, int64_t &response_length, CefString &redirectUrl) {
    DEBUG("JSInterception::GetResponseHeaders: StatusCode: {}, StatusText: {}", (int)status_code, lift::http::to_string(status_code));

    response->SetHeaderMap(responseHeaderMap);
    response_length = download_total;
    // response->SetMimeType(mime_type);
}

void JSInterception::Cancel() {
}

void JSInterception::OnResourceLoadComplete(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                             CefRefPtr<CefRequest> request, CefRefPtr<CefResponse> response,
                                             CefResourceRequestHandler::URLRequestStatus status,
                                             int64_t received_content_length) {
    DEBUG("JSInterception::OnResourceLoadComplete: {} -> {}", (int)status, received_content_length);
}

auto JSInterception::on_lift_complete(lift::request_ptr request, lift::response response) -> void
{
    TRACE("[lift] on_lift_complete, status_code {}", (int)response.status_code());

    status_code = response.status_code();

    if (response.lift_status() == lift::lift_status::success) {
        DEBUG("[lift] Request Completed {} in {} ms", request->url(), response.total_time().count());
        download_data = response.data();

        if (request->url().find("new-hbbtv.zdf.de/static/js/main.") != std::string::npos) {
            // always use HTML5 video object
            // Alt: window.location.pathname.toLowerCase().includes("dash.html")
            // Neu: true
            std::string searchStrHtml5  = "window.location.pathname.toLowerCase().includes(\"dash.html\")";
            std::string replaceStrHtml5 = "true                                                        ";

            auto pos = download_data.find(searchStrHtml5);
            if (pos != std::string::npos) {
                download_data = std::string(download_data.substr(0, pos))
                                + replaceStrHtml5
                                + std::string(download_data.substr(pos + searchStrHtml5.length()));
            }

            // set default autoplay = false:
            // Alt: =n.autoplay
            // Neu: =false
            std::string searchStrAutoplay  = "=n.autoplay";
            std::string replaceStrAutoplay = "=false     ";

            auto posAutoplay = download_data.find(searchStrAutoplay);
            if (posAutoplay != std::string::npos) {
                download_data = std::string(download_data.substr(0, posAutoplay))
                                + replaceStrAutoplay
                                + std::string(download_data.substr(posAutoplay + searchStrAutoplay.length()));
            }

            // save downloaded data
            /*
                FILE* f = fopen("main.js", "wb");
                fwrite(download_data.data(), download_data.length(), 1, f);
                fclose(f);
            */

            download_total = download_data.length();
        } else {
            // copy only
            download_data = response.data();
            download_total = download_data.length();
        }
    } else {
        DEBUG("[lift] Error {}, Code {}, Text {} (Curl {}) in {} ms", request->url(), (int)status_code, lift::http::to_string(status_code), response.network_error_message(), response.total_time().count());
    }

    callback->Continue();
}
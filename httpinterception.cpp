#include <cstdio>
#include <map>
#include <utility>
#include "tools.h"
#include "lift.h"
#include "httpinterception.h"
#include "json.hpp"

PageModifier HttpInterception::modifier;

/*
 * log a header map
 */
void logHeaderMap(std::string prefix, CefResponse::HeaderMap map) {
    if (logger->isTraceEnabled()) {
        for (auto itr = map.begin(); itr != map.end(); ++itr) {
            TRACE("{}: {}: {}", prefix, itr->first.ToString(), itr->second.ToString());
        }
    }
}

// CefResourceRequestHandler
CefResourceRequestHandler::ReturnValue HttpInterception::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) {
    TRACE("HttpInterception::OnBeforeResourceLoad: {}", request->GetURL().ToString());

    LOG_CURRENT_THREAD();

    if (blockThis) {
        return RV_CANCEL;
    }

    this->callback = callback;

    // async version
    LiftUtil::getInstance().get_lift_url(request, 30 * 1000, this);

    // sync version
    // LiftUtil::getInstance().get_lift_url_sync(request, 30 * 1000, this);

    return RV_CONTINUE_ASYNC;
}

HttpInterception::HttpInterception(std::string staticPath, bool blockThis) {
    modifier.init(std::move(staticPath));
    this->blockThis = blockThis;

    offset = 0;
}

bool HttpInterception::Open(CefRefPtr<CefRequest> request, bool& handle_request, CefRefPtr<CefCallback> callback) {
    TRACE("HttpInterception::Open: {}", request->GetURL().ToString());

    handle_request = true;
    return true;
}

bool HttpInterception::Read(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefResourceReadCallback> callback) {
    TRACE("HttpInterception::Read: {}, Size {}, Offset {}", bytes_to_read, download_data.length(), offset);

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

void HttpInterception::GetResponseHeaders(CefRefPtr<CefResponse> response, int64_t &response_length, CefString &redirectUrl) {
    DEBUG("HttpInterception::GetResponseHeaders: StatusCode: {}, StatusText: {}", (int)status_code, lift::http::to_string(status_code));

    response->SetHeaderMap(responseHeaderMap);
    response_length = download_total;
    response->SetMimeType(mime_type);
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

auto HttpInterception::on_lift_complete(lift::request_ptr request, lift::response response) -> void
{
    TRACE("[lift] on_lift_complete, status_code {}", (int)response.status_code());

    status_code = response.status_code();

    if (response.lift_status() == lift::lift_status::success) {
        DEBUG("[lift] Request Completed {} in {} ms", request->url(), response.total_time().count());

        // save downloaded data
        download_data = HttpInterception::modifier.injectAll(response.data());
        download_total = download_data.length();

        TRACE("[lift] Injected data\n{}", download_data);
        TRACE("[lift] Downloaded data, length {}", download_total);

        TRACE("[lift] Response headers:");

        // save response headers
        bool foundContentType = false;
        std::for_each(response.headers().begin(), response.headers().end(), [&](const lift::header &header) {
            TRACE("   {}: {}", header.name(), header.value());

            auto posct = strcasestr(header.name().data(), "content-type");
            auto poscl = strcasestr(header.name().data(), "content-length");

            if(posct != nullptr) {
                if (header.value().find("text/html") != std::string::npos) {
                    TRACE("Set Content-Type text/html");
                    mime_type = "text/html";
                    responseHeaderMap.insert(std::make_pair("Content-Type", "text/html"));
                } else if (header.value().find("application/vnd.hbbtv.xhtml+xml") != std::string::npos) {
                    TRACE("Set Content-Type application/xhtml+xml;charset=UTF-8");
                    mime_type = "application/xhtml+xml";
                    responseHeaderMap.insert(std::make_pair("Content-Type", "application/xhtml+xml;charset=UTF-8"));
                } else {
                    TRACE("Set Content-Type application/xhtml+xml;charset=UTF-8");
                    // default
                    mime_type = "application/xhtml+xml";
                    responseHeaderMap.insert(std::make_pair("Content-Type", "application/xhtml+xml;charset=UTF-8"));
                }

                foundContentType = true;
            } else if (poscl != nullptr) {
                responseHeaderMap.insert(std::make_pair("Content-Length", std::to_string(download_total)));
            } else {
                responseHeaderMap.insert(std::make_pair(std::string{header.name()}, std::string{header.value()}));
            }
        });

        if (!foundContentType) {
            TRACE("Set default Content-Type text/html");

            // default
            mime_type = "text/html";
            responseHeaderMap.insert(std::make_pair("Content-Type", "text/html"));
        }
    } else{
        DEBUG("[lift] Error {} in {} ms", request->url(), response.total_time().count());
    }

    // add CORS header if needed
    responseHeaderMap.insert(std::make_pair("Access-Control-Allow-Origin", "*"));

    callback->Continue();
}

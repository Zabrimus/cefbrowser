#include "tools.h"
#include "trackinginterception.h"

std::string trackingUrlBlockList[] {
        ".nmrodam.com",
        ".ioam.de",
        ".xiti.com",
        ".sensic.net",
        ".tvping.com",
        "tracking.redbutton.de",
        "px.moatads.com",
        "2mdn.net",
        "mefo1.zdf.de",
        ".gvt1.com", // testwise
        ".gvt2.com"  // testwise
};

// CefResourceRequestHandler
CefResourceRequestHandler::ReturnValue TrackingInterception::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) {
    // TRACE("TrackingInterception::OnBeforeResourceLoad: {}", request->GetURL().ToString());

    return RV_CONTINUE_ASYNC;
}

bool TrackingInterception::Open(CefRefPtr<CefRequest> request, bool& handle_request, CefRefPtr<CefCallback> callback) {
    // TRACE("TrackingInterception::Open: {}", request->GetURL().ToString());

    handle_request = true;
    return true;
}

bool TrackingInterception::Read(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefResourceReadCallback> callback) {
    bytes_read = 0;
    return true;
}

void TrackingInterception::GetResponseHeaders(CefRefPtr<CefResponse> response, int64_t &response_length, CefString &redirectUrl) {
    response->SetStatus(404);
    response->SetStatusText("Not Found");
}

void TrackingInterception::Cancel() {
}

void TrackingInterception::OnResourceLoadComplete(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                             CefRefPtr<CefRequest> request, CefRefPtr<CefResponse> response,
                                             CefResourceRequestHandler::URLRequestStatus status,
                                             int64_t received_content_length) {
    DEBUG("TrackingInterception::OnResourceLoadComplete: {} -> {}", (int)status, received_content_length);

    CefResourceRequestHandler::OnResourceLoadComplete(browser, frame, request, response, status, received_content_length);
}

bool TrackingInterception::IsTracker(std::string& url) {
    // check if URL has to be blocked
    size_t found = url.find_first_of(':');
    size_t found2 = url.substr(found + 3).find_first_of('/');

    for (const auto & block : trackingUrlBlockList) {
        if (url.substr(found + 3, found2).find(block) != std::string::npos) {
            return true;
        }
    }

    return false;
}

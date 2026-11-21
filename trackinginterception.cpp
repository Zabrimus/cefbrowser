#include "tools.h"
#include "trackinginterception.h"

std::string trackingUrlBlockList[] {
/*
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
*/
};

// CefResourceRequestHandler
CefResourceRequestHandler::ReturnValue TrackingInterception::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) {
    // TRACE("TrackingInterception::OnBeforeResourceLoad: {}", request->GetURL().ToString());

    // RV_CONTINUE_ASYNC promises that a CefCallback runs later to release the
    // request, and nothing here ever runs one, so the load hung.
    return RV_CONTINUE;
}

bool TrackingInterception::Open(CefRefPtr<CefRequest> request, bool& handle_request, CefRefPtr<CefCallback> callback) {
    // TRACE("TrackingInterception::Open: {}", request->GetURL().ToString());

    handle_request = true;
    return true;
}

bool TrackingInterception::Read(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefResourceReadCallback> callback) {
    // Completion is 0 bytes and false. Returning true means the data arrives
    // later through the callback, which this handler never runs.
    bytes_read = 0;
    return false;
}

void TrackingInterception::GetResponseHeaders(CefRefPtr<CefResponse> response, int64_t &response_length, CefString &redirectUrl) {
    // An empty document rather than an error. This handler also serves
    // BLANK_PAGE, which is a main frame navigation, and a 404 with no body there
    // makes Chromium substitute its own 'site cannot be reached' page - which
    // the plugin then paints over the video, opaque and full screen. Blocked
    // trackers are equally happy with an empty body.
    response->SetStatus(200);
    response->SetStatusText("OK");
    response->SetMimeType("text/html");
    response_length = 0;
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

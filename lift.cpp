#include <chrono>
#include <utility>
#include "lift.h"
#include "cef_includes.h"
#include "logger.h"

lift::client* client;


std::unique_ptr<lift::request> LiftUtil::createRequest(CefRefPtr<CefRequest> request, int timeout) {
    auto _timeout = std::chrono::milliseconds(timeout);
    auto request_ptr = std::make_unique<lift::request>(request->GetURL().ToString(), _timeout);

    // copy request headers
    request_ptr->clear_headers();

    CefRequest::HeaderMap requestHeaderMap;
    request->GetHeaderMap(requestHeaderMap);

    for (auto itr = requestHeaderMap.begin(); itr != requestHeaderMap.end(); ++itr) {
        TRACE("[lift] RequestHeader: {} -> {}", itr->first.ToString(), itr->second.ToString());
        request_ptr->header(itr->first.ToString(), itr->second.ToString());
    }

    if (request->GetMethod().ToString() == "POST") {
        TRACE("[lift] Post Count: {}", request->GetPostData()->GetElementCount());

        CefPostData::ElementVector elements;
        request->GetPostData()->GetElements(elements);
        std::for_each(elements.begin(), elements.end(), [&](const CefRefPtr<CefPostDataElement> &item) {
            TRACE("   Type:  {}", (int)item->GetType());

            if (item->GetType() == PDE_TYPE_FILE) {
                TRACE("   File:  {}", item->GetFile().ToString());
            }

            if (item->GetType() == PDE_TYPE_BYTES) {
                TRACE("   Bytes: {}", item->GetBytesCount());

                char bytes[10000];
                item->GetBytes(item->GetBytesCount(), &bytes);
                TRACE("   Bytes: {}", std::string(bytes));

                request_ptr->data(std::string(bytes));
            }

            if (item->GetType() == PDE_TYPE_EMPTY) {
                TRACE("   Empty Post");
            }
        });
    }

    return request_ptr;
}

auto LiftUtil::get_lift_url(CefRefPtr<CefRequest> request, int timeout, LiftCallback* callback) -> void {
    if (client == nullptr) {
        client = new lift::client;
    }

    TRACE("[lift] URL: {}", request->GetURL().ToString());
    TRACE("[lift] Method: {}", request->GetMethod().ToString());

    // start request
    client->start_request(
            std::move(createRequest(request, timeout)),
            [callback](lift::request_ptr request, lift::response response)
            {
                callback->on_lift_complete(std::move(request), std::move(response));
            });
}

auto LiftUtil::get_lift_url_sync(CefRefPtr<CefRequest> request, int timeout, LiftCallback* callback) -> void {
    if (client == nullptr) {
        client = new lift::client;
    }

    std::unique_ptr<lift::request> liftRequest = createRequest(std::move(request), timeout);

    auto response = liftRequest->perform();
    callback->on_lift_complete(std::move(liftRequest), std::move(response));
}
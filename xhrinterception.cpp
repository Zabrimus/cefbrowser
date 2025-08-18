#include <cstdio>
#include "tools.h"
#include "xhrinterception.h"
#include "json.hpp"

void logLiftResponseHeader(std::string prefix, lift::response& response) {
    if (logger->isTraceEnabled()) {
        std::for_each(response.headers().begin(), response.headers().end(), [&](const lift::header &item) {
            TRACE("{}{}:{}", prefix, item.name(), item.value());
        });
    }
}

// CefResourceRequestHandler
CefResourceRequestHandler::ReturnValue XhrInterception::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) {
    TRACE("XhrInterception::OnBeforeResourceLoad: {}", request->GetURL().ToString());

    this->callback = callback;

    // async version
    LiftUtil::getInstance().get_lift_url(request, 30 * 1000, this);

    // sync version
    // LiftUtil::getInstance().get_lift_url_sync(request, 30 * 1000, this);

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
    // DEBUG("XhrInterception::Read: {}, Size {}, Offset {}", bytes_to_read, client->download_data.length(), client->offset);

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

void XhrInterception::GetResponseHeaders(CefRefPtr<CefResponse> response, int64_t &response_length, CefString &redirectUrl) {
    DEBUG("XhrInterception::GetResponseHeaders: StatusCode: {}, StatusText: {}", (int)status_code, lift::http::to_string(status_code));

    response->SetHeaderMap(responseHeaderMap);
    response_length = download_total;
    // response->SetMimeType(mime_type);
}

void XhrInterception::Cancel() {
}

void XhrInterception::OnResourceLoadComplete(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                             CefRefPtr<CefRequest> request, CefRefPtr<CefResponse> response,
                                             CefResourceRequestHandler::URLRequestStatus status,
                                             int64_t received_content_length) {
    DEBUG("XhrInterception::OnResourceLoadComplete: {} -> {}", (int)status, received_content_length);
}

auto XhrInterception::on_lift_complete(lift::request_ptr request, lift::response response) -> void
{
    TRACE("[lift] on_lift_complete, status_code {}", (int)response.status_code());
    logLiftResponseHeader("  ", response);

    status_code = response.status_code();

    if (response.lift_status() == lift::lift_status::success) {
        DEBUG("[lift] Request Completed {} in {} ms", request->url(), response.total_time().count());

        // hbbtv.zdf.de
        if (logger->isTraceEnabled()) {
            TRACE("Downloaded data:\n{}\n", response.data());
        }

        if (request->url().find("-hbbtv.zdf.de/ds/configuration") != std::string::npos) {
            nlohmann::json dataJson;

            try {
                dataJson = nlohmann::ordered_json::parse(response.data());
            } catch (nlohmann::json::parse_error &e) {
                ERROR("Json Parse error: {}", e.what());
                return;
            }

            if (dataJson.find("dash") != dataJson.end()) {
                dataJson["dash"] = false;
            }

            if (dataJson.find("dashJsInHbbtv") != dataJson.end()) {
                // dataJson["dashJsInHbbtv"] = true;
            }

            // save downloaded data
            download_data = dataJson.dump();
            download_total = download_data.length();
        } else if (request->url().find("hbbtv.zdf.de") != std::string::npos) {
            nlohmann::json dataJson;

            try {
                dataJson = nlohmann::ordered_json::parse(response.data());
            } catch (nlohmann::json::parse_error &e) {
                ERROR("Json Parse error: {}", e.what());
                return;
            }

            // Version 1
            if (dataJson.find("data") != dataJson.end()) {
                dataJson["data"].erase("ageControl");
            }

            // Version 2
            if (dataJson["fsk"]["age"] != nullptr) {
                dataJson["fsk"].erase("age");
            }
            if (dataJson["fsk"] != nullptr) {
                dataJson.erase("fsk");
            }

            // Version 3
            if (dataJson.find("videoMetaData") != dataJson.end()) {
                dataJson["videoMetaData"].erase("fsk");
            }

            // save downloaded data
            download_data = dataJson.dump();
            download_total = download_data.length();
        } else if (request->url().find("tv.ardmediathek.de/dyn/get?id=video") != std::string::npos) {
            nlohmann::json dataJson;

            try {
                dataJson = nlohmann::ordered_json::parse(response.data());
            } catch (nlohmann::json::parse_error &e) {
                ERROR("Json Parse error: {}", e.what());
                return;
            }

            // Version 1
            if (dataJson["video"]["meta"]["maturityContentRating"] != nullptr) {
                // dataJson["video"]["meta"].erase("maturityContentRating");
                // dataJson["video"]["meta"]["maturityContentRating"]["age"] = "6";
                // dataJson["video"]["meta"]["maturityContentRating"]["isBlocked"] = false;
            }

            // save downloaded data
            download_data = dataJson.dump();
            download_total = download_data.length();
        } else {
            // copy only
            download_data = response.data();
            download_total = download_data.length();
        }
    } else {
        DEBUG("[lift] Error {}, Code {}, Text {} (Curl {}) in {} ms", request->url(), (int)status_code, lift::http::to_string(status_code), response.network_error_message(), response.total_time().count());
    }

    // add CORS header if needed
    responseHeaderMap.insert(std::make_pair("Access-Control-Allow-Origin", "*"));

    callback->Continue();
}


/*
XhrRequestClient::XhrRequestClient(CefRefPtr<CefCallback>& resourceCallback) : upload_total(0), download_total(0), offset(0), callback(resourceCallback) {
}

void XhrRequestClient::OnRequestComplete(CefRefPtr<CefURLRequest> request) {
    TRACE("XhrRequestClient::OnRequestComplete:  {}, {}, {}", (int) request->GetRequestStatus(),
          (int) request->GetRequestError(), request->GetResponse()->GetMimeType().ToString());

    request->GetResponse()->GetHeaderMap(headerMap);

    // hbbtv.zdf.de
    if (logger->isTraceEnabled()) {
        TRACE("Downloaded data:\n{}\n", download_data);
    }

    if (request->GetRequest()->GetURL().ToString().find("-hbbtv.zdf.de/ds/configuration") != std::string::npos) {
        nlohmann::json dataJson;

        try {
            dataJson = nlohmann::ordered_json::parse(download_data);
        } catch(nlohmann::json::parse_error& e) {
            ERROR("Json Parse error: {}", e.what());
            return;
        }

        if (dataJson.find("dash") != dataJson.end()) {
            dataJson["dash"] = false;
        }

        if (dataJson.find("dashJsInHbbtv") != dataJson.end()) {
            // dataJson["dashJsInHbbtv"] = true;
        }

        download_data = dataJson.dump();
    } else if (request->GetRequest()->GetURL().ToString().find("hbbtv.zdf.de") != std::string::npos) {
        nlohmann::json dataJson;

        try {
            dataJson = nlohmann::ordered_json::parse(download_data);
        } catch(nlohmann::json::parse_error& e) {
            ERROR("Json Parse error: {}", e.what());
            return;
        }

        // Version 1
        if (dataJson.find("data") != dataJson.end()) {
            dataJson["data"].erase("ageControl");
        }

        // Version 2
        if (dataJson["fsk"]["age"] != nullptr) {
            dataJson["fsk"].erase("age");
        } if (dataJson["fsk"] != nullptr) {
            dataJson.erase("fsk");
        }

        // Version 3
        if (dataJson.find("videoMetaData") != dataJson.end()) {
            dataJson["videoMetaData"].erase("fsk");
        }

        download_data = dataJson.dump();
    } else if (request->GetRequest()->GetURL().ToString().find("tv.ardmediathek.de/dyn/get?id=video") != std::string::npos) {
        nlohmann::json dataJson;

        try {
            dataJson = nlohmann::ordered_json::parse(download_data);
        } catch(nlohmann::json::parse_error& e) {
            ERROR("Json Parse error: {}", e.what());
            return;
        }

        // Version 1
        if (dataJson["video"]["meta"]["maturityContentRating"] != nullptr) {
            // dataJson["video"]["meta"].erase("maturityContentRating");
            // dataJson["video"]["meta"]["maturityContentRating"]["age"] = "6";
            // dataJson["video"]["meta"]["maturityContentRating"]["isBlocked"] = false;
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
    // DEBUG("XhrRequestClient::OnDownloadData: {}: {} -> {}", request->GetRequest()->GetURL().ToString(), data_length, download_data.length());

    std::string downloadChunk = std::string(static_cast<const char*>(data), data_length);
    download_data += downloadChunk;
}

bool XhrRequestClient::GetAuthCredentials(bool isProxy, const CefString &host, int port, const CefString &realm,
                                       const CefString &scheme, CefRefPtr<CefAuthCallback> callback) {
    TRACE("XhrRequestClient::GetAuthCredentials: {}, {}", host.ToString(), port);
    return false;
}
*/
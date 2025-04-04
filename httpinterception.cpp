#include <cstdio>
#include <map>
#include <utility>
#include "tools.h"
#include "httpinterception.h"
#include "json.hpp"

PageModifier HttpInterception::modifier;

void PageModifier::init(std::string sp) {
    if (!cache.empty()) {
        // already filled
        return;
    }

    staticPath = std::move(sp);

    // -----------------------------------------
    // PreJavascript (Fehlt: _dynamic.js)
    // -----------------------------------------
    std::string preJs[] = {"simple-tinyduration.js", "mutation-summary.js", "init.js", "keyhandler.js" };

    std::string preJsResult;
    for (const auto & file : preJs) {
        preJsResult += readFile((staticPath + "/js/" + file).c_str());
    }

    cache.emplace("PRE_JS", std::string("\n<script type=\"text/javascript\">\n//<![CDATA[\n") + preJsResult + "\n// ]]>\n</script>\n");

    // -----------------------------------------
    // PreCSS
    // -----------------------------------------
    std::string preCss[] = { "TiresiasPCfont.css", "videoquirks.css" };

    std::string preCssResult;
    for (const auto & file : preCss) {
        preCssResult += readFile((staticPath + "/css/" + file).c_str());
    }

    cache.emplace("PRE_CSS", std::string("\n<style>\n") + preCssResult + "\n</style>\n");

    // -----------------------------------------
    // PostJavascript (fehlt: _zoom_level.js)
    // -----------------------------------------
    std::string postJs[] = { "video_quirks.js", "videoobserver.js", "hbbtv.js", "initlast.js" };

    std::string postJsResult;
    for (const auto & file : postJs) {
        postJsResult += readFile((staticPath + "/js/" + file).c_str());
    }

    cache.emplace("POST_JS", std::string("\n<script type=\"text/javascript\">\n//<![CDATA[\n") + postJsResult + "\n// ]]>\n</script>\n");
}

std::string PageModifier::insertAfterHead(std::string &origStr, std::string &addStr) {
    std::size_t found = origStr.find("<head");
    if (found == std::string::npos) {
        return origStr;
    }

    found = origStr.find(">", found + 1, 1);
    if (found == std::string::npos) {
        return origStr;
    }

    return origStr.substr(0, found + 1) + addStr + origStr.substr(found + 1);
}

std::string PageModifier::insertBeforeEnd(std::string &origStr, std::string &addStr) {
    std::size_t found = origStr.find("</body>");
    if (found == std::string::npos) {
        return origStr;
    }

    return origStr.substr(0, found) + addStr + origStr.substr(found);
}

std::string PageModifier::getDynamicJs() {
    std::string dynamicJs = readFile((staticPath + "/js/_dynamic.js").c_str());
    return std::string("\n<script type=\"text/javascript\">\n//<![CDATA[\n") + dynamicJs + "\n// ]]>\n</script>\n";
}

std::string PageModifier::getDynamicZoom() {
    std::string dynamicJs = readFile((staticPath + "/js/_zoom_level.js").c_str());
    return std::string("\n<script type=\"text/javascript\">\n//<![CDATA[\n") + dynamicJs + "\n// ]]>\n</script>\n";
}

std::string PageModifier::injectAll(std::string &source) {
    std::string result;

    result = insertAfterHead(source, cache["PRE_JS"]);

    std::string dyn1 = getDynamicJs();
    result = insertAfterHead(result, dyn1);

    result = insertBeforeEnd(result, cache["POST_JS"]);

    std::string dyn2 = getDynamicZoom();
    result = insertBeforeEnd(result, dyn2);

    result = insertAfterHead(result, cache["PRE_CSS"]);

    return result;
}

// CefResourceRequestHandler
CefResourceRequestHandler::ReturnValue HttpInterception::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) {
    TRACE("XhrRequestResponse::OnBeforeResourceLoad: {}", request->GetURL().ToString());

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
    DEBUG("HttpInterception::Read: {}, Size {}, Offset {}", bytes_to_read, client->download_data.length(), client->offset);

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

    CefResponse::HeaderMap responseHeader;
    url_request->GetResponse()->GetHeaderMap(responseHeader);
    for (auto itr = responseHeader.begin(); itr != responseHeader.end(); ++itr) {
        TRACE("ResponseHeader: {} -> {}", itr->first.ToString(), itr->second.ToString());
    }

    CefRequest::HeaderMap requestHeader;
    url_request->GetRequest()->GetHeaderMap(requestHeader);
    for (auto itr = requestHeader.begin(); itr != requestHeader.end(); ++itr) {
        TRACE("RequestHeader: {} -> {}", itr->first.ToString(), itr->second.ToString());
    }

    // TODO: Content-Type ändern, falls erforderlich

    std::string contentType = response->GetHeaderByName("Content-Type").ToString();

    responseHeader.erase("Content-Type");
    responseHeader.insert(std::make_pair("Content-Type","application/xhtml+xml;charset=UTF-8"));

    if (!contentType.empty()) {
        if (contentType.find("text/html") != std::string::npos) {
            response->SetMimeType("text/html");
        } else if (contentType.find("application/vnd.hbbtv.xhtml+xml") != std::string::npos) {
            response->SetMimeType("application/xhtml+xml");
        } else {
            // default
            response->SetMimeType("application/xhtml+xml");
        }
    } else {
        // default
        response->SetMimeType("application/xhtml+xml");
    }

    response->SetHeaderMap(responseHeader);

    response_length = client->download_total;
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

HttpRequestClient::HttpRequestClient(CefRefPtr<CefCallback>& resourceCallback) : download_total(0), offset(0), callback(resourceCallback) {
}

void HttpRequestClient::OnRequestComplete(CefRefPtr<CefURLRequest> request) {
    TRACE("HttpRequestClient::OnRequestComplete:  {}, {}, {}", (int) request->GetRequestStatus(),
          (int) request->GetRequestError(), request->GetResponse()->GetMimeType().ToString());

    // TODO: Seite ändern, falls erforderlich
    download_data = HttpInterception::modifier.injectAll(download_data);
    callback->Continue();
}

void HttpRequestClient::OnDownloadProgress(CefRefPtr<CefURLRequest> request, int64_t current, int64_t total) {
    download_total = total;
}

void HttpRequestClient::OnDownloadData(CefRefPtr<CefURLRequest> request, const void *data, size_t data_length) {
    DEBUG("HttpRequestClient::OnDownloadData: {}: {} -> {}", request->GetRequest()->GetURL().ToString(), data_length, download_data.length());

    std::string downloadChunk = std::string(static_cast<const char*>(data), data_length);
    download_data += downloadChunk;
}

bool HttpRequestClient::GetAuthCredentials(bool isProxy, const CefString &host, int port, const CefString &realm,
                                       const CefString &scheme, CefRefPtr<CefAuthCallback> callback) {
    TRACE("HttpRequestClient::GetAuthCredentials: {}, {}", host.ToString(), port);
    return false;
}
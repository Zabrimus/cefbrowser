#include "requestresponse.h"
#include <cstdio>
#include <fstream>
#include "tools.h"
#include "filetype.cpp"
#include "httplib.h"

std::string static_path;
std::string preJavascript;
std::string postJavascript;
std::string preCSS;
std::string postHTML;

std::string lastContentType;

bool isVideoOrAudio(std::string& data) {
    std::string type;
    std::string mime;

    if (data.length() > 80) {
        FileType(data, type, mime);

        if ((mime.find("video/") != std::string::npos) || (mime.find("audio/") != std::string::npos)) {
            return true;
        }
    }

    return false;
}

std::string readPreJavascript(std::string browserIp, int browserPort) {
    std::string result;
    std::string files[] = {"simple-tinyduration.js", "mutation-summary.js", "init.js", "keyhandler.js", "_dynamic.js" };

    for (const auto & file : files) {
        result += readFile((static_path + "/js/" + file).c_str());
    }

    return std::string("\n<script type=\"text/javascript\">\n//<![CDATA[\n") + result + "\n// ]]>\n</script>\n";
}

std::string readPreCSS(std::string browserIp, int browserPort) {
    std::string result;
    std::string files[] = { "TiresiasPCfont.css", "videoquirks.css" };

    for (const auto & file : files) {
        result += readFile((static_path + "/css/" + file).c_str());
    }

    return std::string("\n<style>\n") + result + "\n</style>\n";
}

std::string readPostJavascript(std::string browserIp, int browserPort) {
    std::string result;
    std::string files[] = { "_zoom_level.js", "video_quirks.js", "videoobserver.js", "hbbtv.js", "initlast.js" };

    for (const auto & file : files) {
        result += readFile((static_path + "/js/" + file).c_str());
    }

    return std::string("\n<script type=\"text/javascript\">\n//<![CDATA[\n") + result + "\n// ]]>\n</script>\n";
}

std::string readPostHTML() {
    std::string result;
    std::string files[] = { "videoimage.html" };

    for (const auto & file : files) {
        result += readFile((static_path + "/js/" + file).c_str());
    }

    return result;
}

std::string insertAfterHead(std::string& origStr, std::string& addStr) {
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

std::string insertBeforeEnd(std::string& origStr, std::string& addStr) {
    std::size_t found = origStr.find("</body>");
    if (found == std::string::npos) {
        return origStr;
    }

    return origStr.substr(0, found) + addStr + origStr.substr(found);
}

// CefResourceRequestHandler
CefResourceRequestHandler::ReturnValue RequestResponse::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) {
    TRACE("RequestResponse::OnBeforeResourceLoad: {}", request->GetURL().ToString());

    // check URL against blocklist
    if (blockThis) {
        return RV_CANCEL;
    }

    std::string url = request->GetURL();

    // check file extension and block all known binary formats.
    auto toBlock = endsWith(url,"view-source:'") ||
                  endsWith(url, ".json") ||
                  endsWith(url, ".js") ||
                  endsWith(url, ".css") ||
                  endsWith(url, ".ico") ||
                  endsWith(url, ".jpg") ||
                  endsWith(url, ".png") ||
                  endsWith(url, ".gif") ||
                  endsWith(url, ".webp") ||
                  endsWith(url, ".m3u8") ||
                  endsWith(url, ".mpd") ||
                  endsWith(url, ".ts") ||
                  endsWith(url, ".mpg") ||
                  endsWith(url, ".mp3") ||
                  endsWith(url, ".mp4") ||
                  endsWith(url, ".m4s") ||
                  endsWith(url, ".mov") ||
                  endsWith(url, ".avi") ||
                  endsWith(url, ".pdf") ||
                  endsWith(url, ".ppt") ||
                  endsWith(url, ".pptx") ||
                  endsWith(url, ".xls") ||
                  endsWith(url, ".xlsx") ||
                  endsWith(url, ".doc") ||
                  endsWith(url, ".docx") ||
                  endsWith(url, ".zip") ||
                  endsWith(url, ".rar") ||
                  endsWith(url, ".woff2") ||
                  endsWith(url, ".svg") ||
                  endsWith(url, ".ogg") ||
                  endsWith(url, ".ogm") ||
                  endsWith(url, ".ttf") ||
                  endsWith(url, ".7z");

    // some special URL parts to check
    if (url.find(".mp4?") != std::string::npos) {
        toBlock = true;
    }

    if (toBlock) {
        TRACE("Url {} blocked", url);
        return RV_CANCEL;
    }

    // receive head of the url request (last fallback)
    CefURLParts _parts;
    CefParseURL(request->GetURL(), _parts);
    std::string path = CefString(&_parts.path);
    std::string port = CefString(&_parts.port);
    std::string host = CefString(&_parts.host);
    std::string scheme = CefString(&_parts.scheme);
    std::string query = CefString(&_parts.query);

    std::string content_type;

    std::string tmp_url = scheme + "://" + host + (port.length() > 0 ? ":" + port : "");
    std::string newUrl;

    httplib::Client _client(tmp_url);
    _client.set_read_timeout(15, 0);

    if (auto res = _client.Head(path)) {
        DEBUG("http-lib: HEAD location {}, path {}, query {}, status {}", res->location, path, query, res->status);

        if (res->status >= 400) {
            DEBUG("Head received status {}", res->status);
            return RV_CONTINUE_ASYNC;
        }

        if (res->status > 300 && res->status < 400) {
            newUrl = res->get_header_value("Location") + (!query.empty() ? "?" + query : "");
            DEBUG("Redirect to new URL {} ", newUrl);

            // redirect
            if (startsWith(newUrl, "/")) {
                newUrl = tmp_url + newUrl;
            }

            request->SetURL(newUrl);
            return RV_CONTINUE;
        }

        if (res->status == 200) {
            DEBUG("Content-Type: {}", res->get_header_value("Content-Type"));
            content_type = res->get_header_value("Content-Type");
        }
    } else {
        auto err = res.error();
        content_type = "";
        DEBUG("HTTP error: url: {}, error: {}", tmp_url, httplib::to_string(err));
    }

    // block everything else, except text/html, application/vnd.hbbtv.xhtml+xml, application/xhtml+xml
    if ( (content_type.find("text/html") == std::string::npos) && (content_type.find("application/vnd.hbbtv.xhtml+xml") == std::string::npos) && (content_type.find("application/xhtml+xml") == std::string::npos) ) {
        TRACE("Url {}, Content Type {} will be handled by the browser itself.", url, content_type);
        return RV_CONTINUE;
    }

    // create request by copying the original one
    CefRefPtr<CefRequest> _request = CefRequest::Create();
    CefRequest::HeaderMap headerMap;
    request->GetHeaderMap(headerMap);

    DEBUG("Vor Request: Location {} (Length {})", newUrl, query);

    _request->Set(request->GetURL(),
                  request->GetMethod(),
                  request->GetPostData(),
                  headerMap);

    client = new RequestClient(callback);
    url_request = CefURLRequest::Create(_request, client.get(), nullptr);

    return RV_CONTINUE_ASYNC;
}

RequestResponse::RequestResponse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                 CefRefPtr<CefRequest> request, bool is_navigation, bool is_download,
                                 const CefString &request_initiator, bool &disable_default_handling,
                                 std::string browserIp, int browserPort, bool blockThis, std::string sp) {
    DEBUG("RequestResponse::RequestResponse: {}, {}, {}, {}", is_navigation, is_download, request->GetURL().ToString(), (int)request->GetResourceType());
    static_path = sp;

    if (!blockThis) {
        preJavascript = readPreJavascript(browserIp, browserPort);
        postJavascript = readPostJavascript(browserIp, browserPort);
        preCSS = readPreCSS(browserIp, browserPort);
        postHTML = readPostHTML();
    }

    this->blockThis = blockThis;
}

bool RequestResponse::Open(CefRefPtr<CefRequest> request, bool& handle_request, CefRefPtr<CefCallback> callback) {
    TRACE("RequestResponse::Open: {}", request->GetURL().ToString());

    handle_request = true;
    return true;
}

bool RequestResponse::Read(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefResourceReadCallback> callback) {
    DEBUG("RequestResponse::Read: {}, Size {}, Offset {}", bytes_to_read, client->download_data.length(), client->offset);

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

void RequestResponse::GetResponseHeaders(CefRefPtr<CefResponse> response, int64_t &response_length, CefString &redirectUrl) {
    DEBUG("RequestResponse::GetResponseHeader: Error1={}, Error2={}, Status1={}, Status2={}, StatusText={}",
            (int)url_request->GetResponse()->GetError(), (int)url_request->GetRequestError(),
            (int)url_request->GetResponse()->GetStatus(), (int)url_request->GetRequestStatus(),
            url_request->GetResponse()->GetStatusText().ToString() );

    if (url_request->GetResponse()->GetStatus() != 200) {
        response->SetStatus(url_request->GetResponse()->GetStatus());
        response->SetStatusText(url_request->GetResponse()->GetStatusText());
        response_length = 0;
        lastContentType = "";

        return;
    }

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

    responseHeader.erase("Content-Type");
    responseHeader.insert(std::make_pair("Content-Type","application/xhtml+xml;charset=UTF-8"));

    response->SetStatus(200);
    response->SetStatusText("OK");

    if (!lastContentType.empty()) {
        if (lastContentType.find("text/html") != std::string::npos) {
            response->SetMimeType("text/html");
        } else if (lastContentType.find("application/vnd.hbbtv.xhtml+xml") != std::string::npos) {
            response->SetMimeType("application/xhtml+xml");
        } else {
            // default
            response->SetMimeType("application/xhtml+xml");
        }
    } else {
        // default
        response->SetMimeType("application/xhtml+xml");
    }

    lastContentType = "";

    response->SetHeaderMap(responseHeader);

    response_length = client->download_total;
}

void RequestResponse::Cancel() {
}

void RequestResponse::OnResourceLoadComplete(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                             CefRefPtr<CefRequest> request, CefRefPtr<CefResponse> response,
                                             CefResourceRequestHandler::URLRequestStatus status,
                                             int64_t received_content_length) {
    DEBUG("RequestResponse::OnResourceLoadComplete: {} -> {}", (int)status, received_content_length);

    CefResourceRequestHandler::OnResourceLoadComplete(browser, frame, request, response, status,
                                                      received_content_length);
}

RequestClient::RequestClient(CefRefPtr<CefCallback>& resourceCallback) : upload_total(0), download_total(0), offset(0), callback(resourceCallback) {
}

void RequestClient::OnRequestComplete(CefRefPtr<CefURLRequest> request) {
    TRACE("RequestClient::OnRequestComplete:  {}, {}, {}", (int)request->GetRequestStatus(), (int)request->GetRequestError(), request->GetResponse()->GetMimeType().ToString());
    DEBUG("RequestClient::OnRequestComplete (before inject), Page Source: {}", download_data);

    // Inject Javascript (header)
    download_data = insertAfterHead(download_data, preJavascript);

    // inject HTML (body)
    download_data = insertBeforeEnd(download_data, postHTML);

    // Inject Javascript (body)
    download_data = insertBeforeEnd(download_data, postJavascript);

    // Inject CSS (header)
    download_data = insertAfterHead(download_data, preCSS);

    DEBUG("RequestClient::OnRequestComplete (after inject), Page Source: {}", download_data);

    request->GetResponse()->GetHeaderMap(headerMap);
    DEBUG("Header by content: {}", request->GetResponse()->GetHeaderByName("Content-Type").ToString());

    lastContentType = request->GetResponse()->GetHeaderByName("Content-Type").ToString();

    callback->Continue();
}

void RequestClient::OnUploadProgress(CefRefPtr<CefURLRequest> request, int64_t current, int64_t total) {
    upload_total = total;
}

void RequestClient::OnDownloadProgress(CefRefPtr<CefURLRequest> request, int64_t current, int64_t total) {
    download_total = total;
}

void RequestClient::OnDownloadData(CefRefPtr<CefURLRequest> request, const void *data, size_t data_length) {
    DEBUG("RequestClient::OnDownloadData: {}: {} -> {}", request->GetRequest()->GetURL().ToString(), data_length, download_data.length());

    std::string downloadChunk = std::string(static_cast<const char*>(data), data_length);

    if (download_data.length() < 80) {
        // check file type
        DEBUG("RequestClient::OnDownloadData: IsVideoOrAudio: {}", isVideoOrAudio(downloadChunk));
        if (isVideoOrAudio(downloadChunk)) {
            callback->Cancel();
            return;
        }
    }

    download_data += downloadChunk;
}

bool RequestClient::GetAuthCredentials(bool isProxy, const CefString &host, int port, const CefString &realm,
                                       const CefString &scheme, CefRefPtr<CefAuthCallback> callback) {
    TRACE("RequestClient::GetAuthCredentials: {}, {}", host.ToString(), port);
    return false;
}

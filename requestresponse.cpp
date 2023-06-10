#include "requestresponse.h"
#include <cstdio>
#include <fstream>
#include "tools.h"
#include "filetype.cpp"


std::string preJavascript;
std::string postJavascript;
std::string preCSS;

std::string urlBlockList[5] {
    ".block.this",
    ".nmrodam.com",
    ".ioam.de",
    ".xiti.com",
    ".sensic.net"
};

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
    std::string files[4] = {"init.js", "keyhandler.js", "font.js", "_dynamic.js" };

    for (const auto & file : files) {
        result += readFile(("js/" + file).c_str());
    }

    std::ofstream _dynamic;
    _dynamic.open ("js/_dynamic_head.js", std::ios_base::trunc);
    _dynamic << result << std::endl;
    _dynamic.close();

    return "\n<script type=\"text/javascript\" src=\"http://" + browserIp + ":" + std::to_string(browserPort) + "/js/_dynamic_head.js\"></script>\n";
}

std::string readPreCSS(std::string browserIp, int browserPort) {
    std::string result;
    std::string files[1] = { "TiresiasPCfont.css" };

    for (const auto & file : files) {
        result += readFile(("css/" + file).c_str());
    }

    std::ofstream _dynamic;
    _dynamic.open ("css/_dynamic.css", std::ios_base::trunc);
    _dynamic << result << std::endl;
    _dynamic.close();

    return "\n<link rel=\"stylesheet\" href=\"http://" + browserIp + ":" + std::to_string(browserPort) + "/css/_dynamic.css\">";

    // return "\n<style>\n" + result + "</style>\n";
}

std::string readPostJavascript(std::string browserIp, int browserPort) {
    std::string result;
    std::string files[2] = { "videoobserver.js", "hbbtv.js" };

    for (const auto & file : files) {
        result += readFile(("js/" + file).c_str());
    }

    std::ofstream _dynamic;
    _dynamic.open ("js/_dynamic_body.js", std::ios_base::trunc);
    _dynamic << result << std::endl;
    _dynamic.close();

    return "\n<script type=\"text/javascript\" src=\"http://" + browserIp + ":" + std::to_string(browserPort) + "/js/_dynamic_body.js\"></script>\n";
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
    std::string url = request->GetURL();
    size_t found = url.find_first_of(':');
    size_t found2 = url.substr(found + 3).find_first_of('/');

    for (const auto & block : urlBlockList) {
        if (url.substr(found + 3, found2).find(block) != std::string::npos) {
            // block
            return RV_CANCEL;
        }
    }

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
        return RV_CANCEL;
    }

    // create request by copying the original one
    CefRefPtr<CefRequest> _request = CefRequest::Create();
    CefRequest::HeaderMap headerMap;
    request->GetHeaderMap(headerMap);

    _request->Set(request->GetURL(),
                  request->GetMethod(),
                  request->GetPostData(),
                  headerMap);

    client = new RequestClient(callback);
    url_request = CefURLRequest::Create(request, client.get(), nullptr);

    return RV_CONTINUE_ASYNC;
}

RequestResponse::RequestResponse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                 CefRefPtr<CefRequest> request, bool is_navigation, bool is_download,
                                 const CefString &request_initiator, bool &disable_default_handling, std::string browserIp, int browserPort) {
    DEBUG("RequestResponse::RequestResponse: {}, {}, {}, {}", is_navigation, is_download, request->GetURL().ToString(), (int)request->GetResourceType());
    preJavascript = readPreJavascript(browserIp, browserPort);
    postJavascript = readPostJavascript(browserIp, browserPort);
    preCSS = readPreCSS(browserIp, browserPort);
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

void RequestResponse::GetResponseHeaders(CefRefPtr<CefResponse> response, int64 &response_length, CefString &redirectUrl) {
    TRACE("RequestResponse::GetResponseHeaders: {}", response->GetURL().ToString());

    response->SetStatus(200);
    response->SetStatusText("OK");
    response->SetMimeType("text/html");
    response->SetHeaderMap(client->headerMap);

    response_length = client->download_total;
}

void RequestResponse::Cancel() {
    DEBUG("RequestResponse::Cancel: url_request");
    url_request->Cancel();
}

void RequestResponse::OnResourceLoadComplete(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                             CefRefPtr<CefRequest> request, CefRefPtr<CefResponse> response,
                                             CefResourceRequestHandler::URLRequestStatus status,
                                             int64 received_content_length) {
    TRACE("RequestResponse::OnResourceLoadComplete: {} -> {}", (int)status, received_content_length);

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

    // Inject Javascript (body)
    download_data = insertBeforeEnd(download_data, postJavascript);

    // Inject CSS
    download_data = insertAfterHead(download_data, preCSS);

    DEBUG("RequestClient::OnRequestComplete (after inject), Page Source: {}", download_data);

    request->GetResponse()->GetHeaderMap(headerMap);
    DEBUG("Header by content: {}", request->GetResponse()->GetHeaderByName("Content-Type").ToString());

    callback->Continue();
}

void RequestClient::OnUploadProgress(CefRefPtr<CefURLRequest> request, int64 current, int64 total) {
    upload_total = total;
}

void RequestClient::OnDownloadProgress(CefRefPtr<CefURLRequest> request, int64 current, int64 total) {
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
        }
    }

    download_data += downloadChunk;
}

bool RequestClient::GetAuthCredentials(bool isProxy, const CefString &host, int port, const CefString &realm,
                                       const CefString &scheme, CefRefPtr<CefAuthCallback> callback) {
    TRACE("RequestClient::GetAuthCredentials: {}, {}", host.ToString(), port);
    return false;
}

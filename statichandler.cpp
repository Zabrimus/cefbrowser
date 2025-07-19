#include <map>
#include <filesystem>
#include <optional>
#include "tools.h"
#include "statichandler.h"
#include "pagemodifier.h"

PageModifier StaticHandler::modifier;

std::string getAbsolutePath(const std::string &root, const std::string &userPath) {
    auto rootPath = std::filesystem::path(root);
    auto finalPath = weakly_canonical(std::filesystem::path(root + userPath));

    auto[rootEnd, nothing] = std::mismatch(rootPath.begin(), rootPath.end(), finalPath.begin());

    if(rootEnd != rootPath.end())
        return "";

    return finalPath.string();
}

// CefResourceRequestHandler
CefResourceRequestHandler::ReturnValue StaticHandler::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) {
    TRACE("StaticHandler::OnBeforeResourceLoad: {}", request->GetURL().ToString());

    return RV_CONTINUE;
}

StaticHandler::StaticHandler(std::string static_path, std::string url) : requestUrl(url), staticPath(static_path), offset(0) {
    modifier.init(static_path);
}

bool StaticHandler::Open(CefRefPtr<CefRequest> request, bool& handle_request, CefRefPtr<CefCallback> callback) {
    TRACE("StaticHandler::Open: {}, {}", request->GetURL().ToString(), staticPath);

    auto fileName = getAbsolutePath(staticPath, requestUrl);

    content = readFile(fileName.c_str());
    mimeType = guessMimeType(requestUrl);

    if (mimeType == "application/xhtml+xml") {
        content = modifier.injectAll(content);
    }

    handle_request = true;
    return true;
}

bool StaticHandler::Read(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefResourceReadCallback> callback) {
    DEBUG("StaticHandler::Read: {}, Size {}, Offset {}", bytes_to_read, content.length(), offset);

    if (content.empty()) {
        bytes_read = 0;
    }

    size_t size = content.length();
    if (offset < size) {
        int transfer_size = std::min(bytes_to_read, static_cast<int>(size - offset));
        memcpy(data_out, content.c_str() + offset, transfer_size);
        offset += transfer_size;

        bytes_read = transfer_size;
        return true;
    }

    return false;
}

void StaticHandler::GetResponseHeaders(CefRefPtr<CefResponse> response, int64_t &response_length, CefString &redirectUrl) {
    CefResponse::HeaderMap responseHeader;

    if (content.empty()) {
        response->SetStatus(404);
        response->SetStatusText("Not Found");
        response_length = 0;
    } else {
        response->SetStatus(200);
        response->SetStatusText("OK");
        response_length = (long)content.length();

        response->SetMimeType(mimeType);
        responseHeader.insert(std::make_pair("Content-Type", mimeType));
        responseHeader.insert(std::make_pair("Content-Length", std::to_string(content.length())));
    }
}

void StaticHandler::Cancel() {
}

void StaticHandler::OnResourceLoadComplete(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                             CefRefPtr<CefRequest> request, CefRefPtr<CefResponse> response,
                                             CefResourceRequestHandler::URLRequestStatus status,
                                             int64_t received_content_length) {
    DEBUG("StaticHandler::OnResourceLoadComplete: {} -> {}", (int)status, received_content_length);
}

std::string StaticHandler::guessMimeType(std::string &name) {
    if (endsWith(name, ".html")) {
        return "application/xhtml+xml";
        // return "text/html";
    } else if (endsWith(name, ".png")) {
        return "image/png";
    } else if (endsWith(name, ".css")) {
        return "text/css";
    } else if (endsWith(name, ".gif")) {
        return "image/gif";
    } else if (endsWith(name, ".xml")) {
        return "application/xml";
    } else if (endsWith(name, ".json")) {
        return "application/json";
    } else if (endsWith(name, ".js")) {
        return "text/javascript";
    }

    return "text/plain";
}

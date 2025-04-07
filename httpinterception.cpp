#include <cstdio>
#include <map>
#include <utility>
#include "httpinterception.h"
#include "json.hpp"

PageModifier HttpInterception::modifier;

HttpInterception::HttpInterception(std::string staticPath, const bool blockThis, const cpr::ConnectionPool& pool) : BaseInterception("HttpInterception", pool) {
    modifier.init(std::move(staticPath));
    this->blockThis = blockThis;

    offset = 0;
}

// CefResourceRequestHandler
CefResourceRequestHandler::ReturnValue HttpInterception::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) {
    TRACE("HttpInterception::OnBeforeResourceLoad: {}", request->GetURL().ToString());

    if (blockThis) {
        return RV_CANCEL;
    }

    return BaseInterception::OnBeforeResourceLoad(browser, frame, request, callback);
}

void HttpInterception::modifyResponse() {
    TRACE("[HttpInterception] modify response, status_code {}", cprResponse.status_code);

    if (cprResponse.status_code < 300) {
        DEBUG("[HttpInterception] Request Completed {} in {} ms", cprResponse.url.str(), cprResponse.elapsed);

        // save downloaded data
        cprResponse.text = HttpInterception::modifier.injectAll(cprResponse.text);

        TRACE("[HttpInterception] Injected data\n{}", cprResponse.text);
        TRACE("[HttpInterception] Downloaded data, length {}", cprResponse.text.length());

        // save response headers
        auto ct = cprResponse.header["Content-Type"];
        auto cl = cprResponse.header["Content-Length"];

        if (!ct.empty()) {
            if (ct.find("text/html") != std::string::npos) {
                TRACE("[HttpInterception] Set Content-Type text/html");
                mimeType = "text/html";
                cprResponse.header["Content-Type"] = "text/html";
            } else if (ct.find("application/vnd.hbbtv.xhtml+xml") != std::string::npos) {
                TRACE("[HttpInterception] Set Content-Type application/xhtml+xml;charset=UTF-8");
                mimeType = "application/xhtml+xml";
                cprResponse.header["Content-Type"] = "application/xhtml+xml;charset=UTF-8";
            } else {
                TRACE("[HttpInterception] Set Content-Type application/xhtml+xml;charset=UTF-8");
                // default
                mimeType = "application/xhtml+xml";
                cprResponse.header["Content-Type"] = "application/xhtml+xml;charset=UTF-8";
            }
        } else {
            mimeType = "text/html";
            cprResponse.header["Content-Type"] = "text/html";
        }

        cprResponse.header["Content-Length"] = std::to_string(cprResponse.text.length());
        cprResponse.header["Access-Control-Allow-Origin"] = "*";
    }
}

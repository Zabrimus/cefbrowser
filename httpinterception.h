#pragma once

#include <string>

#include "baseinterception.h"
#include "cef_includes.h"
#include "json.hpp"
#include "pagemodifier.h"

class HttpRequestClient;

class HttpInterception : public BaseInterception {
public:
    HttpInterception(std::string staticPath, bool blockThis, const cpr::ConnectionPool& pool);

    CefResourceRequestHandler::ReturnValue OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) override;

protected:
    void modifyResponse() override;

private:
    bool blockThis;
    static PageModifier modifier;

private:
    IMPLEMENT_REFCOUNTING(HttpInterception);
};

#pragma once

#include <lift/lift.hpp>
#include "cef_includes.h"

class LiftCallback {
public:
    virtual auto on_lift_complete(lift::request_ptr request_ptr, lift::response response) -> void = 0;
};

class LiftUtil {
public:
    // remove copy constructor and assignment
    LiftUtil(LiftUtil const&) = delete;
    void operator=(LiftUtil const&)  = delete;

    static LiftUtil& getInstance() {
        static LiftUtil instance;
        return instance;
    }

    auto get_lift_url(CefRefPtr<CefRequest> request, int timeout, LiftCallback* callback) -> void;
    auto get_lift_url_sync(CefRefPtr<CefRequest> request, int timeout, LiftCallback* callback) -> void;

private:
    lift::client* client = new lift::client;

    std::unique_ptr<lift::request> createRequest(CefRefPtr<CefRequest> request, int timeout);

    LiftUtil() {};
    ~LiftUtil() {
        delete client;
    }
};

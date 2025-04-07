#pragma once

#include <string>

#include "baseinterception.h"
#include "cef_includes.h"
#include "json.hpp"

class XhrInterception : public BaseInterception {

public:
    explicit XhrInterception(const cpr::ConnectionPool& pool) : BaseInterception("XhrInterception", pool) {};

protected:
    void modifyResponse() override;

private:
    IMPLEMENT_REFCOUNTING(XhrInterception);
};

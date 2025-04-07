#pragma once

#include <string>
#include "cef_includes.h"
#include "baseinterception.h"

class JSInterception : public BaseInterception {

public:
    explicit JSInterception(const cpr::ConnectionPool& pool) : BaseInterception("JSInterception", pool) {};

protected:
    void modifyResponse() override;

private:
    IMPLEMENT_REFCOUNTING(JSInterception);
};

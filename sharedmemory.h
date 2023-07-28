#pragma once

#include <cstdio>
#include <cstdint>

class SharedMemory {
private:
    uint8_t *shmp;

public:
    explicit SharedMemory();
    ~SharedMemory();

    uint8_t* Get();
    bool Write(uint8_t* data, size_t size);
};

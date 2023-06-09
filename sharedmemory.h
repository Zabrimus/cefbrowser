#pragma once

#include <cstdint>

class SharedMemory {
public:
    explicit SharedMemory();
    ~SharedMemory();

    bool write(uint8_t *data, int size);
    void shutdown();

private:
    int shmid;
    uint8_t *shmp;
};

extern SharedMemory sharedMemory;

#pragma once

#include <cstdint>

class SharedMemory {
public:
    explicit SharedMemory();
    ~SharedMemory();

    bool write(uint8_t *data, int size);
    uint8_t* get();

    void shutdown();

private:
    int shmid;
    uint8_t *shmp;
};

extern SharedMemory sharedMemory;

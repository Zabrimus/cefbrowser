#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <cstring>
#include "sharedmemory.h"
#include "logger.h"

const std::string sharedMemoryFile("/cefbrowser");
const int sharedMemorySize = 1920 * 1080 * 4;

SharedMemory::SharedMemory() {
    int shmid = shm_open(sharedMemoryFile.c_str(), O_RDWR, 0666);
    if (shmid < 0) {
        shmid = shm_open(sharedMemoryFile.c_str(), O_EXCL | O_CREAT | O_RDWR, 0666);
        if (shmid >= 0) {
            ftruncate(shmid, sharedMemorySize);
        }
    }

    shmp = (uint8_t*)mmap(nullptr, sharedMemorySize, PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0);
    close(shmid);
}

SharedMemory::~SharedMemory() {
    // munmap(shmp, sharedMemorySize);
}

uint8_t* SharedMemory::Get() {
    return shmp;
}

bool SharedMemory::Write(uint8_t* data, int size) {
    if (size > sharedMemorySize || size <= 0) {
        // abort
        ERROR("Prevent writing %d bytes in buffer of length %d", size, sharedMemorySize);
        return false;
    }

    memcpy(shmp, data, size);

    return true;
}
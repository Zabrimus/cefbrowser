#include <string>
#include <chrono>
#include "ffmpeghandler.h"
#include "logger.h"
#include "vdrremoteclient.h"

void startReaderThread(int fifo, FFmpegHandler* handler, VdrRemoteClient* client) {
    const auto wait_duration = std::chrono::milliseconds(10) ;

    ssize_t bytes;
    char buffer[32712];

    INFO("Start reader thread...");

    while (handler->isRunning()) {
        if ((bytes = read(fifo, buffer, sizeof(buffer))) > 0) {
            if (!client->ProcessTSPacket(std::string(buffer, bytes))) {
                // connection problems? abort transcoding
                ERROR("Unable to connect to vdr. Abort transcoding...");
                handler->stopVideo();
            }
        } else {
            std::this_thread::sleep_for(wait_duration);
        }
    }
}

FFmpegHandler::FFmpegHandler(VdrRemoteClient* client)  : client(client) {
    streamHandler = nullptr;
    readerThread = nullptr;
    readerRunning = false;
    fifo = -1;
}

FFmpegHandler::~FFmpegHandler() {
    readerRunning = false;
    stopVideo();
}

bool FFmpegHandler::streamVideo(std::string url) {
    bool createPipe = false;

    // stop existing video streaming, paranoia
    stopVideo();

    DEBUG("Create FIFO");
    std::string fifoFilename = "/tmp/ffmpegts_vdrtool";

    struct stat sb{};
    if(stat(fifoFilename.c_str(), &sb) != -1) {
        if (!S_ISFIFO(sb.st_mode) != 0) {
            if(remove(fifoFilename.c_str()) != 0) {
                ERROR("File {} exists and is not a pipe. Delete failed. Aborting...\n", fifoFilename.c_str());
                return false;
            } else {
                createPipe = true;
            }
        }
    } else {
        createPipe = true;
    }

    if (createPipe) {
        if (mkfifo(fifoFilename.c_str(), 0666) < 0) {
            ERROR("Unable to create pipe {}. Aborting...\n", fifoFilename.c_str());
            return false;
        }
    }

    // TODO: Evt. Transcoding durchführen. Die Commandline muss generischer werden.
    DEBUG("Start transcoder");
    client->StartVideo(std::string());

    std::string cmdLine = "ffmpeg -re -y -i " + url +  " -c copy -f mpegts " + fifoFilename;
    streamHandler = new TinyProcessLib::Process(cmdLine, "",
        [](const char *bytes, size_t n) {
            DEBUG("{}", std::string(bytes, n));
        },

        [](const char *bytes, size_t n) {
            DEBUG("{}", std::string(bytes, n));
        },
        true
    );

    if ((fifo = open(fifoFilename.c_str(), O_RDONLY)) < 0) {
        ERROR("FFmpegHandler::streamVideo: {}", strerror(errno));
        return false;
    }

    // start reader thread
    DEBUG("Start reader thread");
    readerRunning = true;
    readerThread = new std::thread(startReaderThread, fifo, this, client);
    // readerThread->detach();
    readerThread->join();

    return true;
}

void FFmpegHandler::stopVideo() {
    client->StopVideo();

    readerRunning = false;

    if (fifo > 0) {
        close(fifo);
        fifo = -1;
    }

    if (readerThread != nullptr) {
        readerThread->join();
        readerThread = nullptr;
    }

    if (streamHandler != nullptr) {
        streamHandler->kill();
        streamHandler->get_exit_status();
        streamHandler = nullptr;
    }
}
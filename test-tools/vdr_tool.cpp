#include "common.h"
#include "vdrremoteclient.h"
#include "ffmpeghandler.h"

int main(int argc, char* argv[]) {
    parseConfiguration(argc, argv);

    if (!videoFile.empty()) {
        VdrRemoteClient client(vdrIp, vdrPort);
        FFmpegHandler handler(client);
        handler.streamVideo(videoFile);
    }
}
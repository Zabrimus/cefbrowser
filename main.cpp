#include <thread>
#include <csignal>
#include <getopt.h>
#include "mainapp.h"
#include "sharedmemory.h"
#include "logger.h"
#include "mini/ini.h"
#include "database.h"

const char kProcessType[] = "type";
const char kRendererProcess[] = "renderer";
const char kZygoteProcess[] = "zygote";

// commandline arguments
std::string configFilename;

// log level
int loglevel;

// http server
std::string browserIp;
int browserPort;

std::string transcoderIp;
int transcoderPort;

std::string vdrIp;
int vdrPort;

bool osdqoi;

enum ProcessType {
    PROCESS_TYPE_BROWSER,
    PROCESS_TYPE_RENDERER,
    PROCESS_TYPE_OTHER,
};

CefRefPtr<CefCommandLine> CreateCommandLine(const CefMainArgs& main_args) {
    CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
    command_line->InitFromArgv(main_args.argc, main_args.argv);
    return command_line;
}

ProcessType GetProcessType(const CefRefPtr<CefCommandLine>& command_line) {
    if (!command_line->HasSwitch(kProcessType))
        return PROCESS_TYPE_BROWSER;

    const std::string& process_type = command_line->GetSwitchValue(kProcessType);

    DEBUG("PROCESS_TYPE: {}", process_type);

    if (process_type == kRendererProcess)
        return PROCESS_TYPE_RENDERER;

    if (process_type == kZygoteProcess)
        return PROCESS_TYPE_RENDERER;

    return PROCESS_TYPE_OTHER;
}

void signal_handler(int signal)
{
    database.shutdown();
    CefShutdown();
    exit(1);
}

void parseCommandLine(int argc, char *argv[]) {
    static struct option long_options[] = {
            { "config",      required_argument, nullptr, 'c' },
            { "loglevel",    optional_argument, nullptr, 'l' },
            { "osdqoi",      optional_argument, nullptr, 'q' },
            {nullptr }
    };

    int c, option_index = 0;
    opterr = 0;
    loglevel = 1;
    osdqoi = false;
    while ((c = getopt_long(argc, argv, "qc:l:", long_options, &option_index)) != -1)
    {
        switch (c)
        {
            case 'c':
                configFilename = optarg;
                break;

            case 'l':
                loglevel = atoi(optarg);
                break;

            case 'q':
                osdqoi = true;
                break;

            default:
                return;
        }
    }
}

bool readConfiguration(std::string configFile) {
    mINI::INIFile file(configFile.c_str());
    mINI::INIStructure ini;
    auto result = file.read(ini);

    if (!result) {
        ERROR("Unable to read config file: {}", configFile);
        return false;
    }

    try {
        browserIp = ini["browser"]["http_ip"];
        if (browserIp.empty()) {
            ERROR("http ip (browser) not found");
            return false;
        }

        std::string tmpBrowserPort = ini["browser"]["http_port"];
        if (tmpBrowserPort.empty()) {
            ERROR("http port (browser) not found");
            return false;
        }

        transcoderIp = ini["transcoder"]["http_ip"];
        if (transcoderIp.empty()) {
            ERROR("http ip (transcoder) not found");
            return false;
        }

        std::string tmpTranscoderPort = ini["transcoder"]["http_port"];
        if (tmpTranscoderPort.empty()) {
            ERROR("http port (transcoder) not found");
            return false;
        }

        vdrIp = ini["vdr"]["http_ip"];
        if (vdrIp.empty()) {
            ERROR("http ip (vdr) not found");
            return false;
        }

        std::string tmpVdrPort = ini["vdr"]["http_port"];
        if (tmpVdrPort.empty()) {
            ERROR("http port (vdr) not found");
            return false;
        }

        browserPort = std::stoi(tmpBrowserPort);
        transcoderPort = std::stoi(tmpTranscoderPort);
        vdrPort = std::stoi(tmpVdrPort);
    } catch (...) {
        ERROR("configuration error. aborting...");
        return false;
    }

    return true;
}

std::string getexepath() {
    char result[ PATH_MAX ];
    ssize_t count = readlink( "/proc/self/exe", result, PATH_MAX );
    return std::string(result, static_cast<unsigned long>((count > 0) ? count : 0));
}

int main(int argc, char *argv[]) {
    parseCommandLine(argc, argv);

    switch (loglevel) {
        case 0:  logger->set_level(spdlog::level::critical); break;
        case 1:  logger->set_level(spdlog::level::err); break;
        case 2:  logger->set_level(spdlog::level::info); break;
        case 3:  logger->set_level(spdlog::level::debug); break;
        case 4:  logger->set_level(spdlog::level::trace); break;
        default: logger->set_level(spdlog::level::err); break;
    }

    CefMainArgs main_args(argc, argv);
    CefRefPtr<CefCommandLine> command_line = CreateCommandLine(main_args);

    int exit_code = CefExecuteProcess(main_args, new BrowserApp(vdrIp, vdrPort, transcoderIp, transcoderPort, browserIp, browserPort, osdqoi), nullptr);
    if (exit_code >= 0) {
        return exit_code;
    }

    if (!readConfiguration(configFilename)) {
        return -1;
    }

    // Main browser process
    auto browserApp = new BrowserApp(vdrIp, vdrPort, transcoderIp, transcoderPort, browserIp, browserPort, osdqoi);
    CefRefPtr<BrowserApp> app(browserApp);

    // Specify CEF global settings here.
    CefSettings settings;
    settings.windowless_rendering_enabled = true;
    settings.no_sandbox = true;
    settings.remote_debugging_port = 9222;

    // CefString(&settings.user_agent).FromASCII("HbbTV/1.4.1 (+DL+PVR+DRM;Samsung;SmartTV2015;T-HKM6DEUC-1490.3;;) OsrTvViewer;Chrome");
    CefString(&settings.user_agent).FromASCII("HbbTV/1.2.1 (+DL+PVR+DRM;Samsung;SmartTV2015;T-HKM6DEUC-1490.3;;) OsrTvViewer;Chrome");

    std::string exepath = getexepath();
    std::string cache_path = exepath.substr(0, exepath.find_last_of('/')) + "/cache";
    std::string profile_path = exepath.substr(0, exepath.find_last_of('/')) + "/profile";

    CefString(&settings.cache_path).FromASCII(cache_path.c_str());
    CefString(&settings.user_data_path).FromASCII(profile_path.c_str());

    // Initialize CEF for the browser process. The first browser instance will be created in CefBrowserProcessHandler::OnContextInitialized() after CEF has been initialized.
    CefInitialize(main_args, settings, app, nullptr);

    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = signal_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, nullptr);
    sigaction(SIGQUIT, &sigIntHandler, nullptr);

    if (!command_line->HasSwitch("config")) {
        ERROR("Argument --config not found. Exiting...");
        logger->shutdown();
        return -1;
    }

    // Run the CEF message loop. This will block until CefQuitMessageLoop() is called.
    CefRunMessageLoop();

    database.shutdown();
    CefShutdown();

    return 0;
}
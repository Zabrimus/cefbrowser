#include <thread>
#include <csignal>
#include <getopt.h>
#include "mainapp.h"
#include "logger.h"
#include "mini/ini.h"
#include "database.h"
#include "tools.h"

const char kProcessType[] = "type";
const char kRendererProcess[] = "renderer";
const char kZygoteProcess[] = "zygote";

// commandline arguments
std::string configFilename;
std::string profilePath;
std::string staticPath;

// log level
int loglevel;

// http server
std::string browserIp;
int browserPort;

std::string transcoderIp;
int transcoderPort;

std::string vdrIp;
int vdrPort;

std::string userAgentPath;
std::string browserDbPath;

image_type_enum image_type;

int zoom_width;
int zoom_height;
double zoom_level;

bool use_dirty_recs;

bool bindAll;

CefRefPtr<BrowserApp> app;

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
    currentBrowser->GetHost()->CloseBrowser(true);
    app = nullptr;

    CefQuitMessageLoop();
}

std::string getexepath() {
    char result[ PATH_MAX ];
    ssize_t count = readlink( "/proc/self/exe", result, PATH_MAX );
    return std::string(result, static_cast<unsigned long>((count > 0) ? count : 0));
}

void parseCommandLine(int argc, char *argv[]) {
    static struct option long_options[] = {
            { "config",      required_argument, nullptr, 'c' },
            { "loglevel",    optional_argument, nullptr, 'l' },
            { "osdqoi",      optional_argument, nullptr, 'q' },
            { "zoom",        optional_argument, nullptr, 'z' },
            { "fullosd",     optional_argument, nullptr, 'f' },
            { "profilePath", optional_argument, nullptr, 'p' },
            { "staticPath",  optional_argument, nullptr, 's' },
            { "bindall",     optional_argument, nullptr, 'b' },
            { "uagent",      optional_argument, nullptr, 'u' },
            { nullptr }
    };

    int c, option_index = 0;
    opterr = 0;
    loglevel = 1;
    image_type = NONE;
    zoom_width = 1280;
    zoom_height = 720;
    zoom_level = 1.0f;
    use_dirty_recs = true;
    bindAll = false;

    staticPath = getexepath();
    staticPath = staticPath.substr(0, staticPath.find_last_of('/'));

    while ((c = getopt_long(argc, argv, "qc:l:z:fp:s:bu:", long_options, &option_index)) != -1)
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
                image_type = QOI;
                break;

            case 'z':
                zoom_width = atoi(optarg);
                switch (zoom_width) {
                    case 1280:
                        zoom_height = 720;
                        zoom_level = 1.0f;
                        break;

                    case 1920:
                        zoom_height = 1080;
                        zoom_level = 1.5f;
                        break;

                    case 2560:
                        zoom_height = 1440;
                        zoom_level = 2.0f;
                        break;

                    case 3840:
                        zoom_height = 2160;
                        zoom_level = 3.0f;
                        break;

                    default: // illegal value
                        ERROR("zoom value {} is not defined.", zoom_width);
                        zoom_width = 1280;
                        zoom_height = 720;
                        zoom_level = 1.0f;
                        break;
                }
                break;

            case 'f':
                use_dirty_recs = false;
                break;

            case 'p':
                profilePath = optarg;
                break;

            case 's':
                staticPath = optarg;
                break;

            case 'b':
                bindAll = true;
                break;

            case 'u':
                userAgentPath = optarg;
                break;

            case 'd':
                browserDbPath = optarg;
                break;

            default:
                // return;
                break;
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

int main(int argc, char *argv[]) {
    // set some default values
    zoom_width = 1280;
    zoom_height = 720;
    zoom_level = 1.0f;

    parseCommandLine(argc, argv);

    switch (loglevel) {
        case 0:  logger->set_level(spdlog::level::critical); break;
        case 1:  logger->set_level(spdlog::level::err); break;
        case 2:  logger->set_level(spdlog::level::info); break;
        case 3:  logger->set_level(spdlog::level::debug); break;
        case 4:  logger->set_level(spdlog::level::trace); break;
        default: logger->set_level(spdlog::level::err); break;
    }

    BrowserParameter bParameter;

    CefMainArgs main_args(argc, argv);
    CefRefPtr<CefCommandLine> command_line = CreateCommandLine(main_args);

    int exit_code = CefExecuteProcess(main_args, new BrowserApp(bParameter), nullptr);
    if (exit_code >= 0) {
        return exit_code;
    }

    if (!readConfiguration(configFilename)) {
        return -1;
    }

    // set all parameters again: new forked process, zygote
    bParameter.vdrIp = vdrIp;
    bParameter.vdrPort = vdrPort;
    bParameter.transcoderIp = transcoderIp;
    bParameter.transcoderPort = transcoderPort;
    bParameter.browserIp = browserIp;
    bParameter.browserPort = browserPort;
    bParameter.image_type = image_type;
    bParameter.zoom_width = zoom_width;
    bParameter.zoom_height = zoom_height;
    bParameter.use_dirty_recs = use_dirty_recs;
    bParameter.static_path = staticPath;
    bParameter.bindAll = bindAll;
    bParameter.user_agent_path = userAgentPath;

    // Main browser process
    auto browserApp = new BrowserApp(bParameter);
    app = browserApp;

    // Specify CEF global settings here.
    CefSettings settings;
    settings.windowless_rendering_enabled = true;
    settings.no_sandbox = true;
    settings.remote_debugging_port = 9222;

    // CefString(&settings.user_agent).FromASCII("HbbTV/1.4.1 (+DL+PVR+DRM;Samsung;SmartTV2015;T-HKM6DEUC-1490.3;;) OsrTvViewer;Chrome");
    // CefString(&settings.user_agent).FromASCII("HbbTV/1.2.1 (+DL+PVR+DRM;Samsung;SmartTV2015;T-HKM6DEUC-1490.3;;) OsrTvViewer;Chrome");
    CefString(&settings.user_agent).FromASCII("HbbTV/1.2.1 (+DL+PVR;Samsung;SmartTV2015;T-HKM6DEUC-1490.3;;) OsrTvViewer;Chrome");

    // check if a custom profile exists, otherwise use the default values
    std::string exepath = getexepath();

    if (profilePath.empty()) {
        std::string profile_path = exepath.substr(0, exepath.find_last_of('/')) + "/profile";
        CefString(&settings.root_cache_path).FromASCII(profile_path.c_str());
    } else {
        CefString(&settings.root_cache_path).FromASCII(profilePath.c_str());
    }

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

    // write zoom level script
    std::ofstream _zoom_level;
    _zoom_level.open (staticPath + "/js/_zoom_level.js", std::ios_base::trunc);
    _zoom_level << "document.body.style[\"zoom\"] = " << zoom_level << ";\n" << std::endl;
    _zoom_level << "document.body.style[\"width\"] = \"" << zoom_width << "px\";\n" << std::endl;
    _zoom_level << "document.body.style[\"height\"] = \"" << zoom_height << "px\";\n" << std::endl;
    _zoom_level.close();

    // Run the CEF message loop. This will block until CefQuitMessageLoop() is called.
    CefRunMessageLoop();
    CefShutdown();

    return 0;
}
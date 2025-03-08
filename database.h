#pragma once

#include <string>
#include "mini/ini.h"
#include "sqlite3.h"

class Database {
public:
    Database();
    ~Database();

    void readUserAgents(std::string path);

    bool insertHbbtv(std::string json);
    bool insertChannel(std::string json);

    std::string getRedButtonUrl(std::string channelId);
    std::string getChannel(std::string channelId);
    std::string getMainApp(std::string channelId);
    std::string getUserAgent(std::string channelId);

    void shutdown();
    void printFatal();

    std::string getAppUrl(const std::string basicString, const std::string basicString1);

private:
    void createTables();

private:
    sqlite3 *db;

    sqlite3_stmt* insertHbbtvStmt;
    sqlite3_stmt* insertChannelStmt;

    mINI::INIStructure userAgents;
};

extern Database database;
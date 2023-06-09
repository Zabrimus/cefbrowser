#pragma once

#include <string>
#include "sqlite3.h"

class Database {
public:
    Database();
    ~Database();

    bool insertHbbtv(std::string json);
    bool insertChannel(std::string json);

    std::string getRedButtonUrl(std::string channelId);
    std::string getChannel(std::string channelId);
    std::string getMainApp(std::string channelId);

private:
    void createTables();

private:
    sqlite3 *db;

    sqlite3_stmt* insertHbbtvStmt;
    sqlite3_stmt* insertChannelStmt;
};

extern Database database;
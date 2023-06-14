#include "database.h"
#include "cef_includes.h"
#include "logger.h"

std::string insertHbbtvSql = R"(
                         INSERT INTO HBBTV_URLS (CHANNEL_ID, CHANNEL_NAME, APPLICATION_ID, CONTROL_CODE, APP_NAME, URL_BASE, URL_LOC, URL_EXT)
                         VALUES (?,?,?,?,?,?,?,?);
                      )";

std::string insertChannelSql = R"(
                         INSERT INTO CHANNELS (CHANNEL_ID, CHANNEL_JSON)
                         VALUES (?,?);
                      )";


int unused_callback(void *NotUsed, int argc, char **argv, char **azColName) {
   return 0;
}

Database::Database() {
    int rc = sqlite3_open_v2("database/hbbtv_urls.db", &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);

    if (rc) {
        ERROR("DB Error: {}", sqlite3_errmsg(db));
        sqlite3_close(db);
        db = nullptr;

        return;
    }

    createTables();

    sqlite3_prepare_v3(db, insertHbbtvSql.c_str(), (int)insertHbbtvSql.length(), SQLITE_PREPARE_PERSISTENT, &insertHbbtvStmt, nullptr);
    sqlite3_prepare_v3(db, insertChannelSql.c_str(), (int)insertChannelSql.length(), SQLITE_PREPARE_PERSISTENT, &insertChannelStmt, nullptr);
}

Database::~Database() {
    shutdown();
}

void Database::shutdown() {
    sqlite3_finalize(insertHbbtvStmt);
    sqlite3_finalize(insertChannelStmt);

    if (db) {
        sqlite3_close(db);
    }
}

bool Database::insertHbbtv(std::string json) {
    CefRefPtr<CefValue> messageJson = CefParseJSON(json.c_str(), json.length(), JSON_PARSER_ALLOW_TRAILING_COMMAS);
    CefRefPtr<CefDictionaryValue> dict = messageJson->GetDictionary();

    CefString channelId = dict->GetString("channelId");
    CefString channelName = dict->GetString("channelName");
    int applicationId = dict->GetInt("applicationId");
    int controlCode = dict->GetInt("controlCode");
    CefString name = dict->GetString("name");
    CefString urlBase = dict->GetString("urlBase");
    CefString urlLoc = dict->GetString("urlLoc");
    CefString urlExt = dict->GetString("urlExt");

    sqlite3_bind_text(insertHbbtvStmt, 1, channelId.ToString().c_str(), (int)channelId.length(), SQLITE_TRANSIENT);
    sqlite3_bind_text(insertHbbtvStmt, 2, channelName.ToString().c_str(), (int)channelName.length(), SQLITE_TRANSIENT);
    sqlite3_bind_int(insertHbbtvStmt,  3, applicationId);
    sqlite3_bind_int(insertHbbtvStmt,  4, controlCode);
    sqlite3_bind_text(insertHbbtvStmt, 5, name.ToString().c_str(), (int)name.length(), SQLITE_TRANSIENT);
    sqlite3_bind_text(insertHbbtvStmt, 6, urlBase.ToString().c_str(), (int)urlBase.length(), SQLITE_TRANSIENT);

    if (urlLoc.empty()) {
        sqlite3_bind_null(insertHbbtvStmt, 7);
    } else {
        sqlite3_bind_text(insertHbbtvStmt, 7, urlLoc.ToString().c_str(), (int)urlLoc.length(), SQLITE_TRANSIENT);
    }

    if (urlExt.empty()) {
        sqlite3_bind_null(insertHbbtvStmt, 8);
    } else {
        sqlite3_bind_text(insertHbbtvStmt, 8, urlExt.ToString().c_str(), (int)urlExt.length(), SQLITE_TRANSIENT);
    }

    sqlite3_step(insertHbbtvStmt);
    sqlite3_clear_bindings(insertHbbtvStmt);
    sqlite3_reset(insertHbbtvStmt);

    return true;
}

bool Database::insertChannel(std::string json) {
    CefRefPtr<CefValue> messageJson = CefParseJSON(json.c_str(), json.length(), JSON_PARSER_ALLOW_TRAILING_COMMAS);
    CefRefPtr<CefDictionaryValue> dict = messageJson->GetDictionary();

    CefString channelId = dict->GetString("channelId");

    sqlite3_bind_text(insertChannelStmt, 1, channelId.ToString().c_str(), (int)channelId.length(), SQLITE_TRANSIENT);
    sqlite3_bind_text(insertChannelStmt, 2, json.c_str(), (int)json.length(), SQLITE_TRANSIENT);

    sqlite3_step(insertChannelStmt);
    sqlite3_clear_bindings(insertChannelStmt);
    sqlite3_reset(insertChannelStmt);

    return true;
}

std::string Database::getRedButtonUrl(std::string channelId) {
    std::string sql = R"(
        SELECT url_base || ifnull(URL_loc,'') || ifnull(url_ext, '') url
        FROM HBBTV_URLS
        WHERE CHANNEL_ID = ?
        AND CONTROL_CODE = 1;
    )";

    sqlite3_stmt* stmt;

    sqlite3_prepare(db, sql.c_str(), (int)sql.length(), &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, channelId.c_str(), (int)channelId.length(), SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    char* url = (char*)sqlite3_column_text(stmt, 0);

    std::string result(url, sqlite3_column_bytes(stmt, 0));

    sqlite3_finalize(stmt);

    return result;
}

std::string Database::getChannel(std::string channelId) {
    std::string sql = R"(
        SELECT CHANNEL_JSON
        FROM CHANNELS
        WHERE CHANNEL_ID = ?;
    )";

    sqlite3_stmt* stmt;

    sqlite3_prepare(db, sql.c_str(), (int)sql.length(), &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, channelId.c_str(), (int)channelId.length(), SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    char* url = (char*)sqlite3_column_text(stmt, 0);

    std::string result(url, sqlite3_column_bytes(stmt, 0));

    sqlite3_finalize(stmt);

    return result;
}

std::string Database::getMainApp(std::string channelId) {
    return std::string();
}


void Database::createTables() {
    char *errMsg = nullptr;

    std::string sqlHbbtvUrls = R"(
                         CREATE TABLE HBBTV_URLS (
                             CHANNEL_ID     TEXT     NOT NULL,
                             CHANNEL_NAME   TEXT     NOT NULL,
                             APPLICATION_ID INTEGER  NOT NULL,
                             CONTROL_CODE   INTEGER  NOT NULL,
                             APP_NAME       TEXT     NOT NULL,
                             URL_BASE       TEXT     NOT NULL,
                             URL_LOC        TEXT     NULL,
                             URL_EXT        TEXT     NULL,

                             PRIMARY KEY(CHANNEL_ID, APPLICATION_ID, CONTROL_CODE)
                         );
                      )";

    sqlite3_exec(db, sqlHbbtvUrls.c_str(), unused_callback, nullptr, &errMsg);

    std::string sqlChannels = R"(
                         CREATE TABLE CHANNELS (
                             CHANNEL_ID     TEXT     NOT NULL,
                             CHANNEL_JSON   TEXT     NOT NULL,

                             PRIMARY KEY(CHANNEL_ID)
                         );
                      )";

    sqlite3_exec(db, sqlChannels.c_str(), unused_callback, nullptr, &errMsg);
}

std::string Database::getAppUrl(const std::string channelId, const std::string appId) {
    std::string sql = R"(
            SELECT url_base || ifnull(URL_loc,'') || ifnull(url_ext, '') url
              FROM HBBTV_URLS
             WHERE CHANNEL_ID = ?
               AND APPLICATION_ID = ?;
         )";

    sqlite3_stmt* stmt;

    sqlite3_prepare(db, sql.c_str(), (int)sql.length(), &stmt, nullptr);

    sqlite3_bind_text(stmt, 1, channelId.c_str(), (int)channelId.length(), SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, std::atoi(appId.c_str()));

    sqlite3_step(stmt);
    char* url = (char*)sqlite3_column_text(stmt, 0);

    std::string result(url, sqlite3_column_bytes(stmt, 0));

    sqlite3_finalize(stmt);

    return result;
}

Database database;
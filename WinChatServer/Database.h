#pragma once

#include <string>
#include <vector>
#include <memory>

#include <mysql/jdbc.h>


// 一条聊天记录
struct ChatRecord
{
    std::string sender;
    std::string receiver;
    std::string content;
    std::string sendTime;
};


class Database
{
public:

    Database(
        const std::string& host,
        const std::string& username,
        const std::string& password,
        const std::string& database
    );


    // 注册用户
    bool registerUser(
        const std::string& username,
        const std::string& password
    );


    // 登录验证
    bool loginUser(
        const std::string& username,
        const std::string& password
    );


    // 保存聊天记录
    bool saveMessage(
        const std::string& sender,
        const std::string& receiver,
        const std::string& content
    );


    // 查询两个人之间的聊天记录
    std::vector<ChatRecord> getHistory(
        const std::string& user1,
        const std::string& user2
    );


private:

    sql::mysql::MySQL_Driver* driver;

    std::string host;
    std::string dbUsername;
    std::string dbPassword;
    std::string databaseName;


    // 创建一次数据库连接
    std::unique_ptr<sql::Connection>
        createConnection();
};
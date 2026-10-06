#pragma once
#include <string>

struct Config
{
    std::string dbHost;
    std::string dbUser;
    std::string dbPassword;
    std::string dbName;
};


// 从配置文件读取数据库配置
Config loadConfig(const std::string& filename);
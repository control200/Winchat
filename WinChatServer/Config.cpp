#include "Config.h"

#include <fstream>
#include <iostream>
#include <stdexcept>


Config loadConfig(const std::string& filename)
{
    std::ifstream file(filename);

    if (!file.is_open())
    {
        throw std::runtime_error(
            "Cannot open config file: " + filename
        );
    }


    Config config;

    std::string line;


    while (std::getline(file, line))
    {
        // 忽略空行
        if (line.empty())
        {
            continue;
        }

        // 忽略注释
        if (line[0] == '#')
        {
            continue;
        }


        size_t pos = line.find('=');

        if (pos == std::string::npos)
        {
            continue;
        }


        std::string key =
            line.substr(0, pos);

        std::string value =
            line.substr(pos + 1);


        if (key == "DB_HOST")
        {
            config.dbHost = value;
        }
        else if (key == "DB_USER")
        {
            config.dbUser = value;
        }
        else if (key == "DB_PASSWORD")
        {
            config.dbPassword = value;
        }
        else if (key == "DB_NAME")
        {
            config.dbName = value;
        }
    }


    if (config.dbHost.empty() ||
        config.dbUser.empty() ||
        config.dbPassword.empty() ||
        config.dbName.empty())
    {
        throw std::runtime_error(
            "Database configuration is incomplete."
        );
    }


    return config;
}
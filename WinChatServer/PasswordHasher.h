#pragma once

#include <string>


struct PasswordData
{
    std::string hash;
    std::string salt;

    unsigned long long iterations;
};


class PasswordHasher
{
public:

    static PasswordData hashPassword(
        const std::string& password
    );


    static bool verifyPassword(
        const std::string& password,
        const std::string& storedHash,
        const std::string& storedSalt,
        unsigned long long iterations
    );
};
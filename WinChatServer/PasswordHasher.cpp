#include "PasswordHasher.h"

#include <windows.h>
#include <bcrypt.h>

#include <vector>
#include <sstream>
#include <iomanip>
#include <stdexcept>

#pragma comment(lib, "Bcrypt.lib")


namespace
{
    constexpr int SALT_SIZE = 16;

    constexpr int HASH_SIZE = 32;

    constexpr unsigned long long
        DEFAULT_ITERATIONS = 100000;


    std::string bytesToHex(
        const std::vector<unsigned char>& data)
    {
        std::ostringstream oss;

        for (unsigned char byte : data)
        {
            oss
                << std::hex
                << std::setw(2)
                << std::setfill('0')
                << static_cast<int>(byte);
        }

        return oss.str();
    }


    std::vector<unsigned char>
        hexToBytes(
            const std::string& hex)
    {
        std::vector<unsigned char> bytes;


        for (size_t i = 0;
            i < hex.length();
            i += 2)
        {
            std::string byteString =
                hex.substr(i, 2);


            unsigned char byte =
                static_cast<unsigned char>(
                    std::stoi(
                        byteString,
                        nullptr,
                        16
                    )
                    );


            bytes.push_back(byte);
        }


        return bytes;
    }


    std::vector<unsigned char>
        deriveKey(
            const std::string& password,
            const std::vector<unsigned char>& salt,
            unsigned long long iterations)
    {
        BCRYPT_ALG_HANDLE algorithm = nullptr;


        NTSTATUS status =
            BCryptOpenAlgorithmProvider(
                &algorithm,
                BCRYPT_SHA256_ALGORITHM,
                nullptr,
                BCRYPT_ALG_HANDLE_HMAC_FLAG
            );


        if (status < 0)
        {
            throw std::runtime_error(
                "BCryptOpenAlgorithmProvider failed."
            );
        }


        std::vector<unsigned char> hash(
            HASH_SIZE
        );


        status =
            BCryptDeriveKeyPBKDF2(
                algorithm,

                reinterpret_cast<PUCHAR>(
                    const_cast<char*>(
                        password.data()
                        )
                    ),

                static_cast<ULONG>(
                    password.size()
                    ),

                const_cast<PUCHAR>(
                    salt.data()
                    ),

                static_cast<ULONG>(
                    salt.size()
                    ),

                iterations,

                hash.data(),

                static_cast<ULONG>(
                    hash.size()
                    ),

                0
            );


        BCryptCloseAlgorithmProvider(
            algorithm,
            0
        );


        if (status < 0)
        {
            throw std::runtime_error(
                "BCryptDeriveKeyPBKDF2 failed."
            );
        }


        return hash;
    }
}


// ========================================
// 创建密码 Hash
// ========================================

PasswordData
PasswordHasher::hashPassword(
    const std::string& password)
{
    std::vector<unsigned char> salt(
        SALT_SIZE
    );


    NTSTATUS status =
        BCryptGenRandom(
            nullptr,
            salt.data(),
            static_cast<ULONG>(
                salt.size()
                ),
            BCRYPT_USE_SYSTEM_PREFERRED_RNG
        );


    if (status < 0)
    {
        throw std::runtime_error(
            "BCryptGenRandom failed."
        );
    }


    std::vector<unsigned char> hash =
        deriveKey(
            password,
            salt,
            DEFAULT_ITERATIONS
        );


    PasswordData result;

    result.hash =
        bytesToHex(hash);

    result.salt =
        bytesToHex(salt);

    result.iterations =
        DEFAULT_ITERATIONS;


    return result;
}


// ========================================
// 验证密码
// ========================================

bool PasswordHasher::verifyPassword(
    const std::string& password,
    const std::string& storedHash,
    const std::string& storedSalt,
    unsigned long long iterations)
{
    std::vector<unsigned char> salt =
        hexToBytes(
            storedSalt
        );


    std::vector<unsigned char> hash =
        deriveKey(
            password,
            salt,
            iterations
        );


    std::string calculatedHash =
        bytesToHex(hash);


    return calculatedHash ==
        storedHash;
}
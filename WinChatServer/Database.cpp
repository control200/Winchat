#include "Database.h"
#include "PasswordHasher.h"

#include <iostream>


Database::Database(
    const std::string& host,
    const std::string& username,
    const std::string& password,
    const std::string& database)
{
    this->host = host;
    this->dbUsername = username;
    this->dbPassword = password;
    this->databaseName = database;

    driver =
        sql::mysql::get_mysql_driver_instance();
}


// 创建数据库连接
std::unique_ptr<sql::Connection>
Database::createConnection()
{
    std::unique_ptr<sql::Connection> conn(
        driver->connect(
            host,
            dbUsername,
            dbPassword
        )
    );

    conn->setSchema(databaseName);

    return conn;
}


// ==============================
// 注册用户
// ==============================

//bool Database::registerUser(
//    const std::string& username,
//    const std::string& password)
//{
//    try
//    {
//        std::unique_ptr<sql::Connection> conn =
//            createConnection();
//
//        std::unique_ptr<sql::PreparedStatement> stmt(
//            conn->prepareStatement(
//                "INSERT INTO users(username, password) "
//                "VALUES (?, ?)"
//            )
//        );
//
//        stmt->setString(1, username);
//        stmt->setString(2, password);
//
//        stmt->executeUpdate();
//
//        return true;
//    }
//    catch (const sql::SQLException& e)
//    {
//        // 1062 = UNIQUE重复
//        if (e.getErrorCode() == 1062)
//        {
//            return false;
//        }
//
//        std::cout
//            << "[Database] Register error: "
//            << e.what()
//            << std::endl;
//
//        return false;
//    }
//}

bool Database::registerUser(
    const std::string& username,
    const std::string& password)
{
    try
    {
        PasswordData passwordData =
            PasswordHasher::hashPassword(
                password
            );


        std::unique_ptr<sql::Connection> conn =
            createConnection();


        std::unique_ptr<sql::PreparedStatement> stmt(
            conn->prepareStatement(
                "INSERT INTO users "
                "(username, "
                "password_hash, "
                "password_salt, "
                "password_iterations) "
                "VALUES (?, ?, ?, ?)"
            )
        );


        stmt->setString(
            1,
            username
        );

        stmt->setString(
            2,
            passwordData.hash
        );

        stmt->setString(
            3,
            passwordData.salt
        );

        stmt->setUInt64(
            4,
            passwordData.iterations
        );


        stmt->executeUpdate();


        return true;
    }
    catch (const sql::SQLException& e)
    {
        if (e.getErrorCode() == 1062)
        {
            return false;
        }


        std::cout
            << "[Database] Register error: "
            << e.what()
            << std::endl;

        return false;
    }
    catch (const std::exception& e)
    {
        std::cout
            << "[Password] Error: "
            << e.what()
            << std::endl;

        return false;
    }
}


// ==============================
// 登录验证
// ==============================

//bool Database::loginUser(
//    const std::string& username,
//    const std::string& password)
//{
//    try
//    {
//        std::unique_ptr<sql::Connection> conn =
//            createConnection();
//
//        std::unique_ptr<sql::PreparedStatement> stmt(
//            conn->prepareStatement(
//                "SELECT password "
//                "FROM users "
//                "WHERE username = ?"
//            )
//        );
//
//        stmt->setString(1, username);
//
//        std::unique_ptr<sql::ResultSet> result(
//            stmt->executeQuery()
//        );
//
//        if (!result->next())
//        {
//            return false;
//        }
//
//        std::string dbPassword =
//            result->getString("password");
//
//        return dbPassword == password;
//    }
//    catch (const sql::SQLException& e)
//    {
//        std::cout
//            << "[Database] Login error: "
//            << e.what()
//            << std::endl;
//
//        return false;
//    }
//}

bool Database::loginUser(
    const std::string& username,
    const std::string& password)
{
    try
    {
        std::unique_ptr<sql::Connection> conn =
            createConnection();


        std::unique_ptr<sql::PreparedStatement> stmt(
            conn->prepareStatement(
                "SELECT "
                "password_hash, "
                "password_salt, "
                "password_iterations "
                "FROM users "
                "WHERE username = ?"
            )
        );


        stmt->setString(
            1,
            username
        );


        std::unique_ptr<sql::ResultSet> result(
            stmt->executeQuery()
        );


        if (!result->next())
        {
            return false;
        }


        std::string storedHash =
            result->getString(
                "password_hash"
            );


        std::string storedSalt =
            result->getString(
                "password_salt"
            );


        unsigned long long iterations =
            result->getUInt64(
                "password_iterations"
            );


        return
            PasswordHasher::verifyPassword(
                password,
                storedHash,
                storedSalt,
                iterations
            );
    }
    catch (const sql::SQLException& e)
    {
        std::cout
            << "[Database] Login error: "
            << e.what()
            << std::endl;

        return false;
    }
    catch (const std::exception& e)
    {
        std::cout
            << "[Password] Error: "
            << e.what()
            << std::endl;

        return false;
    }
}


// ==============================
// 保存聊天记录
// ==============================

bool Database::saveMessage(
    const std::string& sender,
    const std::string& receiver,
    const std::string& content)
{
    try
    {
        std::unique_ptr<sql::Connection> conn =
            createConnection();

        std::unique_ptr<sql::PreparedStatement> stmt(
            conn->prepareStatement(
                "INSERT INTO messages"
                "(sender, receiver, content) "
                "VALUES (?, ?, ?)"
            )
        );

        stmt->setString(1, sender);
        stmt->setString(2, receiver);
        stmt->setString(3, content);

        stmt->executeUpdate();

        return true;
    }
    catch (const sql::SQLException& e)
    {
        std::cout
            << "[Database] Save message error: "
            << e.what()
            << std::endl;

        return false;
    }
}

std::vector<ChatRecord>
Database::getHistory(
    const std::string& user1,
    const std::string& user2)
{
    std::vector<ChatRecord> history;

    try
    {
        std::unique_ptr<sql::Connection> conn =
            createConnection();


        std::unique_ptr<sql::PreparedStatement> stmt(
            conn->prepareStatement(
                "SELECT sender, receiver, content, send_time "
                "FROM messages "
                "WHERE "
                "(sender = ? AND receiver = ?) "
                "OR "
                "(sender = ? AND receiver = ?) "
                "ORDER BY send_time ASC "
                "LIMIT 50"
            )
        );


        stmt->setString(1, user1);
        stmt->setString(2, user2);

        stmt->setString(3, user2);
        stmt->setString(4, user1);


        std::unique_ptr<sql::ResultSet> result(
            stmt->executeQuery()
        );


        while (result->next())
        {
            ChatRecord record;

            record.sender =
                result->getString("sender");

            record.receiver =
                result->getString("receiver");

            record.content =
                result->getString("content");

            record.sendTime =
                result->getString("send_time");

            history.push_back(record);
        }
    }
    catch (const sql::SQLException& e)
    {
        std::cout
            << "[Database] History error: "
            << e.what()
            << std::endl;
    }

    return history;
}
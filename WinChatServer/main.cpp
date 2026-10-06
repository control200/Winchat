#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <memory>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "Database.h"
#include "Config.h"


#pragma comment(lib, "Ws2_32.lib")


// ================================
// 全局数据
// ================================


//std::unordered_map<std::string, std::string> users;


// 在线用户
// username -> socket
std::unordered_map<std::string, SOCKET> onlineUsers;


// 保护 onlineUsers
std::mutex onlineUsersMutex;


// 保护控制台输出
std::mutex coutMutex;


// 防止多个线程同时向 Socket 发送数据
std::mutex sendMutex;

std::unique_ptr<Database> database;


// ================================
// 发送完整消息
// ================================

bool sendMessage(SOCKET socket, const std::string& message)
{
    std::lock_guard<std::mutex> lock(sendMutex);

    // 每条消息使用 \n 作为结束标志
    std::string data = message + "\n";

    int totalSent = 0;
    int dataLength = static_cast<int>(data.size());

    while (totalSent < dataLength)
    {
        int sent = send(
            socket,
            data.c_str() + totalSent,
            dataLength - totalSent,
            0
        );

        if (sent == SOCKET_ERROR)
        {
            return false;
        }

        totalSent += sent;
    }

    return true;
}


// ================================
// 处理 REGISTER
// ================================

void handleRegister(
    SOCKET clientSocket,
    const std::string& username,
    const std::string& password)
{
    if (database->registerUser(
        username,
        password))
    {
        sendMessage(
            clientSocket,
            "SYS|Register successful."
        );

        std::lock_guard<std::mutex> lock(coutMutex);

        std::cout
            << "[REGISTER] "
            << username
            << std::endl;
    }
    else
    {
        sendMessage(
            clientSocket,
            "ERR|Username already exists."
        );
    }
}


// ================================
// 处理 LOGIN
// ================================

bool handleLogin(
    SOCKET clientSocket,
    const std::string& username,
    const std::string& password,
    std::string& currentUser)
{
    // 查询MySQL
    if (!database->loginUser(
        username,
        password))
    {
        sendMessage(
            clientSocket,
            "ERR|Invalid username or password."
        );

        return false;
    }


    // 检查是否已经在线
    {
        std::lock_guard<std::mutex> lock(onlineUsersMutex);

        if (onlineUsers.find(username)
            != onlineUsers.end())
        {
            sendMessage(
                clientSocket,
                "ERR|User is already online."
            );

            return false;
        }

        onlineUsers[username] =
            clientSocket;
    }


    currentUser = username;


    sendMessage(
        clientSocket,
        "SYS|Login successful."
    );


    {
        std::lock_guard<std::mutex> lock(coutMutex);

        std::cout
            << "[LOGIN] "
            << username
            << std::endl;
    }


    return true;
}


// ================================
// 私聊
// ================================

void handlePrivateChat(
    SOCKET clientSocket,
    const std::string& currentUser,
    const std::string& targetUser,
    const std::string& content)
{
    SOCKET targetSocket = INVALID_SOCKET;

    {
        std::lock_guard<std::mutex> lock(onlineUsersMutex);

        auto it = onlineUsers.find(targetUser);

        if (it == onlineUsers.end())
        {
            sendMessage(
                clientSocket,
                "ERR|Target user is not online."
            );

            return;
        }

        targetSocket = it->second;
    }

    // 给目标用户发送
    sendMessage(
        targetSocket,
        "CHAT|" + currentUser + "|" + content
    );

    database->saveMessage(
        currentUser,
        targetUser,
        content
    );

    {
        std::lock_guard<std::mutex> lock(coutMutex);

        std::cout
            << "[PRIVATE] "
            << currentUser
            << " -> "
            << targetUser
            << ": "
            << content
            << std::endl;
    }
}


// ================================
// 群聊
// ================================

void handleBroadcast(
    SOCKET clientSocket,
    const std::string& currentUser,
    const std::string& content)
{
    std::vector<SOCKET> receivers;

    {
        std::lock_guard<std::mutex> lock(onlineUsersMutex);

        for (const auto& pair : onlineUsers)
        {
            // 不发送给自己
            if (pair.second != clientSocket)
            {
                receivers.push_back(pair.second);
            }
        }
    }

    for (SOCKET socket : receivers)
    {
        sendMessage(
            socket,
            "CHAT|" + currentUser + "|" + content
        );
    }

    {
        std::lock_guard<std::mutex> lock(coutMutex);

        std::cout
            << "[ALL] "
            << currentUser
            << ": "
            << content
            << std::endl;
    }
    database->saveMessage(
        currentUser,
        "ALL",
        content
    );
}


void handleListUsers(SOCKET clientSocket)
{
    std::string userList;

    {
        std::lock_guard<std::mutex> lock(onlineUsersMutex);

        for (const auto& user : onlineUsers)
        {
            if (!userList.empty())
            {
                userList += ", ";
            }

            userList += user.first;
        }
    }

    if (userList.empty())
    {
        userList = "No users online.";
    }

    sendMessage(
        clientSocket,
        "USERS|" + userList
    );
}


void handleHistory(
    SOCKET clientSocket,
    const std::string& currentUser,
    const std::string& targetUser)
{
    std::vector<ChatRecord> history =
        database->getHistory(
            currentUser,
            targetUser
        );


    if (history.empty())
    {
        sendMessage(
            clientSocket,
            "SYS|No chat history."
        );

        return;
    }


    sendMessage(
        clientSocket,
        "SYS|Chat history with " + targetUser + ":"
    );


    for (const ChatRecord& record : history)
    {
        std::string message =
            "HISTORY|"
            + record.sendTime
            + "|"
            + record.sender
            + "|"
            + record.content;


        sendMessage(
            clientSocket,
            message
        );
    }
}


// ================================
// 解析客户端消息
// ================================

void processMessage(
    SOCKET clientSocket,
    std::string& currentUser,
    const std::string& message)
{
    // 找第一个 |
    size_t first = message.find('|');

    if (first == std::string::npos)
    {
        sendMessage(
            clientSocket,
            "ERR|Invalid message format."
        );

        return;
    }


    // 找第二个 |
    size_t second = message.find('|', first + 1);

    if (second == std::string::npos)
    {
        sendMessage(
            clientSocket,
            "ERR|Invalid message format."
        );

        return;
    }


    std::string type =
        message.substr(0, first);

    std::string param1 =
        message.substr(
            first + 1,
            second - first - 1
        );

    std::string param2 =
        message.substr(second + 1);


    // ----------------
    // REGISTER
    // ----------------

    if (type == "REGISTER")
    {
        handleRegister(
            clientSocket,
            param1,
            param2
        );

        return;
    }


    // ----------------
    // LOGIN
    // ----------------

    if (type == "LOGIN")
    {
        if (!currentUser.empty())
        {
            sendMessage(
                clientSocket,
                "ERR|You are already logged in."
            );

            return;
        }

        handleLogin(
            clientSocket,
            param1,
            param2,
            currentUser
        );

        return;
    }


    // ----------------
    // CHAT
    // ----------------

    if (type == "CHAT")
    {
        // 必须先登录
        if (currentUser.empty())
        {
            sendMessage(
                clientSocket,
                "ERR|Please login first."
            );

            return;
        }

        std::string targetUser = param1;
        std::string content = param2;

        if (targetUser == "ALL")
        {
            handleBroadcast(
                clientSocket,
                currentUser,
                content
            );
        }
        else
        {
            handlePrivateChat(
                clientSocket,
                currentUser,
                targetUser,
                content
            );
        }

        return;
    }

    if (type == "USERS")
    {
        if (currentUser.empty())
        {
            sendMessage(
                clientSocket,
                "ERR|Please login first."
            );

            return;
        }

        handleListUsers(clientSocket);

        return;
    }

    if (type == "HISTORY")
    {
        if (currentUser.empty())
        {
            sendMessage(
                clientSocket,
                "ERR|Please login first."
            );

            return;
        }


        handleHistory(
            clientSocket,
            currentUser,
            param1
        );

        return;
    }


    // 未知类型
    sendMessage(
        clientSocket,
        "ERR|Unknown message type."
    );
}


// ================================
// 客户端线程
// ================================

void handleClient(SOCKET clientSocket)
{
    std::string currentUser;

    {
        std::lock_guard<std::mutex> lock(coutMutex);

        std::cout
            << "New client connected."
            << std::endl;
    }


    char buffer[1024];


    std::string pending;


    while (true)
    {
        int bytesReceived = recv(
            clientSocket,
            buffer,
            sizeof(buffer),
            0
        );

        if (bytesReceived <= 0)
        {
            break;
        }


        // 将新数据追加到 pending
        pending.append(
            buffer,
            bytesReceived
        );


        // 不断寻找 \n
        size_t pos;

        while ((pos = pending.find('\n'))
            != std::string::npos)
        {
            // 取出完整一条消息
            std::string message =
                pending.substr(0, pos);


            // 删除已经处理的数据
            pending.erase(
                0,
                pos + 1
            );


            if (!message.empty())
            {
                processMessage(
                    clientSocket,
                    currentUser,
                    message
                );
            }
        }
    }


    // ================================
    // 客户端断开
    // ================================

    if (!currentUser.empty())
    {
        std::lock_guard<std::mutex> lock(onlineUsersMutex);

        auto it = onlineUsers.find(currentUser);

        if (it != onlineUsers.end() &&
            it->second == clientSocket)
        {
            onlineUsers.erase(it);
        }
    }


    closesocket(clientSocket);


    {
        std::lock_guard<std::mutex> lock(coutMutex);

        if (!currentUser.empty())
        {
            std::cout
                << "[LOGOUT] "
                << currentUser
                << std::endl;
        }
        else
        {
            std::cout
                << "Client disconnected."
                << std::endl;
        }
    }
}


// ================================
// main
// ================================

int main()
{

    try
    {
        Config config =
            loadConfig("config.ini");


        database =
            std::make_unique<Database>(
                config.dbHost,
                config.dbUser,
                config.dbPassword,
                config.dbName
            );


        std::cout
            << "Database configuration loaded."
            << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout
            << "Config error: "
            << e.what()
            << std::endl;

        return 1;
    }

    // 1. 初始化 Winsock
    WSADATA wsaData;

    int result = WSAStartup(
        MAKEWORD(2, 2),
        &wsaData
    );

    if (result != 0)
    {
        std::cout
            << "WSAStartup failed."
            << std::endl;

        return 1;
    }


    // 2. 创建监听 Socket
    SOCKET listenSocket = socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP
    );

    if (listenSocket == INVALID_SOCKET)
    {
        std::cout
            << "socket failed."
            << std::endl;

        WSACleanup();

        return 1;
    }


    // 3. 设置服务器地址
    sockaddr_in serverAddr{};

    serverAddr.sin_family =
        AF_INET;

    serverAddr.sin_addr.s_addr =
        htonl(INADDR_ANY);

    serverAddr.sin_port =
        htons(8888);


    // 4. bind
    result = bind(
        listenSocket,
        reinterpret_cast<sockaddr*>(&serverAddr),
        sizeof(serverAddr)
    );

    if (result == SOCKET_ERROR)
    {
        std::cout
            << "bind failed."
            << std::endl;

        closesocket(listenSocket);

        WSACleanup();

        return 1;
    }


    // 5. listen
    result = listen(
        listenSocket,
        SOMAXCONN
    );

    if (result == SOCKET_ERROR)
    {
        std::cout
            << "listen failed."
            << std::endl;

        closesocket(listenSocket);

        WSACleanup();

        return 1;
    }


    std::cout
        << "============================"
        << std::endl;

    std::cout
        << " WinChat Server"
        << std::endl;

    std::cout
        << " Port: 8888"
        << std::endl;

    std::cout
        << "============================"
        << std::endl;


    // 6. 接受客户端
    while (true)
    {
        SOCKET clientSocket = accept(
            listenSocket,
            nullptr,
            nullptr
        );

        if (clientSocket == INVALID_SOCKET)
        {
            continue;
        }


        std::thread clientThread(
            handleClient,
            clientSocket
        );

        clientThread.detach();
    }


    closesocket(listenSocket);

    WSACleanup();

    return 0;
}
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")


std::atomic<bool> running = true;

std::mutex sendMutex;


// ================================
// 发送消息
// ================================

bool sendMessage(
    SOCKET socket,
    const std::string& message)
{
    std::lock_guard<std::mutex> lock(sendMutex);

    std::string data =
        message + "\n";

    int totalSent = 0;

    int dataLength =
        static_cast<int>(data.size());


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
// 显示服务器消息
// ================================

void showMessage(
    const std::string& message)
{
    // SYS|xxx
    if (message.rfind("SYS|", 0) == 0)
    {
        std::cout
            << "\n[System] "
            << message.substr(4)
            << std::endl;

        return;
    }

    if (message.rfind("USERS|", 0) == 0)
    {
        std::cout
            << "\n[Online Users] "
            << message.substr(6)
            << std::endl;

        return;
    }


    // ERR|xxx
    if (message.rfind("ERR|", 0) == 0)
    {
        std::cout
            << "\n[Error] "
            << message.substr(4)
            << std::endl;

        return;
    }


    // CHAT|username|content
    if (message.rfind("CHAT|", 0) == 0)
    {
        size_t first =
            message.find('|');

        size_t second =
            message.find(
                '|',
                first + 1
            );

        if (second != std::string::npos)
        {
            std::string username =
                message.substr(
                    first + 1,
                    second - first - 1
                );

            std::string content =
                message.substr(
                    second + 1
                );

            std::cout
                << "\n["
                << username
                << "] "
                << content
                << std::endl;

            return;
        }
    }

    if (message.rfind("HISTORY|", 0) == 0)
    {
        size_t first =
            message.find('|');

        size_t second =
            message.find(
                '|',
                first + 1
            );

        size_t third =
            message.find(
                '|',
                second + 1
            );


        if (second != std::string::npos &&
            third != std::string::npos)
        {
            std::string time =
                message.substr(
                    first + 1,
                    second - first - 1
                );


            std::string sender =
                message.substr(
                    second + 1,
                    third - second - 1
                );


            std::string content =
                message.substr(
                    third + 1
                );


            std::cout
                << "\n["
                << time
                << "] "
                << sender
                << ": "
                << content
                << std::endl;
        }

        return;
    }

    std::cout
        << "\n"
        << message
        << std::endl;
}


// ================================
// 接收线程
// ================================

void receiveMessages(
    SOCKET clientSocket)
{
    char buffer[1024];

    std::string pending;


    while (running)
    {
        int bytesReceived = recv(
            clientSocket,
            buffer,
            sizeof(buffer),
            0
        );

        if (bytesReceived <= 0)
        {
            std::cout
                << "\nDisconnected from server."
                << std::endl;

            running = false;

            break;
        }


        pending.append(
            buffer,
            bytesReceived
        );


        size_t pos;

        while ((pos = pending.find('\n'))
            != std::string::npos)
        {
            std::string message =
                pending.substr(
                    0,
                    pos
                );


            pending.erase(
                0,
                pos + 1
            );


            if (!message.empty())
            {
                showMessage(message);
            }
        }
    }
}


// ================================
// main
// ================================

int main()
{
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


    // 2. 创建 Socket
    SOCKET clientSocket = socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP
    );

    if (clientSocket == INVALID_SOCKET)
    {
        WSACleanup();

        return 1;
    }


    // 3. 服务器地址
    sockaddr_in serverAddr{};

    serverAddr.sin_family =
        AF_INET;

    serverAddr.sin_port =
        htons(8888);

    inet_pton(
        AF_INET,
        "127.0.0.1",
        &serverAddr.sin_addr
    );


    // 4. connect
    result = connect(
        clientSocket,
        reinterpret_cast<sockaddr*>(&serverAddr),
        sizeof(serverAddr)
    );

    if (result == SOCKET_ERROR)
    {
        std::cout
            << "Connect failed."
            << std::endl;

        closesocket(clientSocket);

        WSACleanup();

        return 1;
    }


    std::cout
        << "============================"
        << std::endl;

    std::cout
        << " WinChat Client"
        << std::endl;

    std::cout
        << "============================"
        << std::endl;


    std::cout
        << "\nCommands:"
        << std::endl;

    std::cout
        << "/register username password"
        << std::endl;

    std::cout
        << "/login username password"
        << std::endl;

    std::cout
        << "/msg username message"
        << std::endl;

    std::cout
        << "/all message"
        << std::endl;

    std::cout
        << "/users"
        << std::endl;

    std::cout
        << "/history username"
        << std::endl;

    std::cout
        << "/exit"
        << std::endl;

    std::cout
        << std::endl;


    // 接收线程
    std::thread receiverThread(
        receiveMessages,
        clientSocket
    );


    // ================================
    // 主线程读取用户输入
    // ================================

    while (running)
    {
        std::string input;

        std::cout << "> ";

        std::getline(
            std::cin,
            input
        );


        if (!running)
        {
            break;
        }


        // ----------------
        // exit
        // ----------------

        if (input == "/exit")
        {
            running = false;

            break;
        }


        // ----------------
        // register
        // ----------------

        if (input.rfind("/register ", 0) == 0)
        {
            std::string data =
                input.substr(10);


            size_t space =
                data.find(' ');


            if (space == std::string::npos)
            {
                std::cout
                    << "Usage: /register username password"
                    << std::endl;

                continue;
            }


            std::string username =
                data.substr(
                    0,
                    space
                );

            std::string password =
                data.substr(
                    space + 1
                );


            sendMessage(
                clientSocket,
                "REGISTER|" +
                username +
                "|" +
                password
            );

            continue;
        }


        // ----------------
        // login
        // ----------------

        if (input.rfind("/login ", 0) == 0)
        {
            std::string data =
                input.substr(7);


            size_t space =
                data.find(' ');


            if (space == std::string::npos)
            {
                std::cout
                    << "Usage: /login username password"
                    << std::endl;

                continue;
            }


            std::string username =
                data.substr(
                    0,
                    space
                );

            std::string password =
                data.substr(
                    space + 1
                );


            sendMessage(
                clientSocket,
                "LOGIN|" +
                username +
                "|" +
                password
            );

            continue;
        }


        // ----------------
        // private message
        // ----------------

        if (input.rfind("/msg ", 0) == 0)
        {
            std::string data =
                input.substr(5);


            size_t space =
                data.find(' ');


            if (space == std::string::npos)
            {
                std::cout
                    << "Usage: /msg username message"
                    << std::endl;

                continue;
            }


            std::string username =
                data.substr(
                    0,
                    space
                );

            std::string content =
                data.substr(
                    space + 1
                );


            sendMessage(
                clientSocket,
                "CHAT|" +
                username +
                "|" +
                content
            );

            continue;
        }


        // ----------------
        // broadcast
        // ----------------

        if (input.rfind("/all ", 0) == 0)
        {
            std::string content =
                input.substr(5);


            sendMessage(
                clientSocket,
                "CHAT|ALL|" +
                content
            );

            continue;
        }

        if (input == "/users")
        {
            sendMessage(
                clientSocket,
                "USERS||"
            );

            continue;
        }

        if (input.rfind("/history ", 0) == 0)
        {
            std::string username =
                input.substr(9);


            if (username.empty())
            {
                std::cout
                    << "Usage: /history username"
                    << std::endl;

                continue;
            }


            sendMessage(
                clientSocket,
                "HISTORY|"
                + username
                + "|"
            );


            continue;
        }

        std::cout
            << "Unknown command."
            << std::endl;
    }


    // ================================
    // 关闭
    // ================================

    shutdown(
        clientSocket,
        SD_BOTH
    );

    closesocket(clientSocket);


    if (receiverThread.joinable())
    {
        receiverThread.join();
    }


    WSACleanup();

    return 0;
}
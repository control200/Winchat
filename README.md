# WinChat

一个基于 C++ 的简单聊天程序，采用 C/S 架构实现。

## 功能

- 用户注册、登录
- 私聊和群聊
- 查看在线用户
- 查询聊天记录
- MySQL 保存用户和消息数据
- 多客户端同时连接

## 技术

- C++17
- Windows API / Winsock2
- TCP Socket
- std::thread / std::mutex
- MySQL 8.0
- MySQL Connector/C++
- Visual Studio 2026

## 项目结构

```text
WinChat/
├── WinChatServer/
│   ├── main.cpp
│   ├── Database.cpp
│   ├── Database.h
│   ├── Config.cpp
│   ├── Config.h
│   ├── PasswordHasher.cpp
│   └── PasswordHasher.h
├── WinChatClient/
│   ├── main.cpp
│   ├── ChatClient.cpp
│   └── ChatClient.h
└── sql/
    └── winchat.sql
```

## 使用

先在 MySQL 中执行 `sql/winchat.sql` 创建数据库。

将 `config.example.ini` 复制为 `config.ini`，填写本机 MySQL 配置：

```ini
DB_HOST=tcp://127.0.0.1:3306
DB_USER=root
DB_PASSWORD=your_password
DB_NAME=winchat
```

然后使用 Visual Studio 2026 编译并运行。

先启动 `WinChatServer`，再启动一个或多个 `WinChatClient`。

## 客户端命令

```text
/register username password
/login username password
/msg username message
/all message
/users
/history username
/exit
```

## 说明

服务器使用 Winsock2 进行 TCP 通信，每个客户端由独立线程处理。

用户信息和聊天记录保存在 MySQL 中，密码经过哈希处理后再写入数据库。

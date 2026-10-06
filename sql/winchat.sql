CREATE DATABASE IF NOT EXISTS winchat
CHARACTER SET utf8mb4
COLLATE utf8mb4_unicode_ci;

USE winchat;


-- =========================
-- users
-- =========================

CREATE TABLE IF NOT EXISTS users
(
    id INT AUTO_INCREMENT PRIMARY KEY,

    username VARCHAR(50)
        NOT NULL
        UNIQUE,

    password_hash CHAR(64)
        NOT NULL,

    password_salt CHAR(32)
        NOT NULL,

    password_iterations BIGINT
        NOT NULL,

    created_at TIMESTAMP
        DEFAULT CURRENT_TIMESTAMP
);


-- =========================
-- messages
-- =========================

CREATE TABLE IF NOT EXISTS messages
(
    id INT AUTO_INCREMENT PRIMARY KEY,

    sender VARCHAR(50)
        NOT NULL,

    receiver VARCHAR(50)
        NOT NULL,

    content TEXT
        NOT NULL,

    send_time TIMESTAMP
        DEFAULT CURRENT_TIMESTAMP
);


CREATE INDEX idx_messages_sender_receiver
ON messages(sender, receiver);

CREATE INDEX idx_messages_send_time
ON messages(send_time);
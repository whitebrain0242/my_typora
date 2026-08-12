-- LEGACY v7.3 installer. For this version use MYSQL_RUN_THIS_V8_0.sql.
CREATE DATABASE IF NOT EXISTS chatroom
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_bin;

CREATE USER IF NOT EXISTS 'chatroom'@'127.0.0.1'
    IDENTIFIED BY 'replace_with_a_strong_password';

ALTER USER 'chatroom'@'127.0.0.1'
    IDENTIFIED BY 'replace_with_a_strong_password';

GRANT SELECT, INSERT, UPDATE, DELETE
    ON chatroom.*
    TO 'chatroom'@'127.0.0.1';

FLUSH PRIVILEGES;

USE chatroom;

SOURCE sql/001_create_users.sql;
SOURCE sql/002_create_friends_and_events.sql;
SOURCE sql/003_create_messages.sql;

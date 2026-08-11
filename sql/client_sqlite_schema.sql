-- Reference schema only.
-- chat_client creates these tables automatically through SqliteClient::open().

PRAGMA journal_mode=WAL;
PRAGMA synchronous=NORMAL;
PRAGMA foreign_keys=ON;

CREATE TABLE IF NOT EXISTS private_messages (
    account_username TEXT NOT NULL,
    server_message_id INTEGER NOT NULL,
    peer_username TEXT NOT NULL,
    sender_username TEXT NOT NULL,
    recipient_username TEXT NOT NULL,
    content TEXT NOT NULL,
    received_at_unix_ms INTEGER NOT NULL,
    outgoing INTEGER NOT NULL CHECK (outgoing IN (0,1)),
    offline_delivery INTEGER NOT NULL CHECK (offline_delivery IN (0,1)),
    PRIMARY KEY (account_username, server_message_id)
);

CREATE INDEX IF NOT EXISTS idx_local_private_peer
ON private_messages(
    account_username,
    peer_username,
    server_message_id
);

CREATE TABLE IF NOT EXISTS group_messages (
    account_username TEXT NOT NULL,
    server_message_id INTEGER NOT NULL,
    group_name TEXT NOT NULL,
    sender_username TEXT NOT NULL,
    content TEXT NOT NULL,
    received_at_unix_ms INTEGER NOT NULL,
    outgoing INTEGER NOT NULL CHECK (outgoing IN (0,1)),
    offline_delivery INTEGER NOT NULL CHECK (offline_delivery IN (0,1)),
    PRIMARY KEY (account_username, server_message_id)
);

CREATE INDEX IF NOT EXISTS idx_local_group_name
ON group_messages(
    account_username,
    group_name,
    server_message_id
);

CREATE TABLE IF NOT EXISTS file_transfers (
    account_username TEXT NOT NULL,
    server_transfer_id INTEGER NOT NULL,
    scope TEXT NOT NULL,
    peer_username TEXT NOT NULL DEFAULT '',
    group_name TEXT NOT NULL DEFAULT '',
    sender_username TEXT NOT NULL,
    file_name TEXT NOT NULL,
    local_path TEXT NOT NULL,
    file_size INTEGER NOT NULL,
    sha256_hex TEXT NOT NULL,
    received_at_unix_ms INTEGER NOT NULL,
    outgoing INTEGER NOT NULL CHECK (outgoing IN (0,1)),
    PRIMARY KEY (account_username, server_transfer_id)
);

CREATE INDEX IF NOT EXISTS idx_local_files_account
ON file_transfers(
    account_username,
    server_transfer_id
);

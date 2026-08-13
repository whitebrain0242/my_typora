# MySQL Setup — v8.4

## Existing v8.3 database

```bash
sudo mysql chatroom \
  < sql/006_create_friend_blocks.sql
```

## Fresh install

```bash
sudo mysql \
  < MYSQL_RUN_THIS_V8_4.sql
```

## New table

```text
friend_blocks
```

Columns:

```text
blocker_username
blocked_username
created_at
```

Primary key:

```text
(blocker_username, blocked_username)
```

## Prepared Statements

`friend_blocks` access methods:

```text
is_friend_blocked
add_friend_block
remove_friend_block
list_blocked_friends
```

全部使用：

```text
MYSQL_STMT
?
MYSQL_BIND
```

## Remove friendship

`remove_friendship()` 现在是事务：

```text
DELETE friendship
↓
DELETE directional block rows
↓
COMMIT
```

避免好友已经删除却留下“好友屏蔽设置”。

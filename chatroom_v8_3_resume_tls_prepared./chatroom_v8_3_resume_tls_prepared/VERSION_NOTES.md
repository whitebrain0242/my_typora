# v8.3 Version Notes

## Added

- TLS 1.2/1.3 transport
- server certificate/private-key configuration
- client CA + IP/hostname verification
- nonblocking `SSL_accept` inside SubReactor
- `SSL_read` / `SSL_write` TcpConnection path
- verified TLS Reactor tests
- resumable private file upload
- resumable group file upload
- resumable private/group download
- server `.part + .resume.pb` checkpoints
- official Protobuf `FileUploadResumeState`
- SQLite `pending_uploads`
- SQLite `partial_downloads`
- automatic upload resume after LOGIN
- `RESUME_UPLOADS`
- `FILE_RESUME_REQUEST`
- `FILE_RESUME_START`
- `FILE_PAUSED`
- MySQL `MYSQL_STMT` abstraction
- typed `MYSQL_BIND` parameters/results
- MySQL prepared-statement source contract test

## Removed from MySQL layer

- `escape()`
- dynamic SQL value concatenation
- `mysql_real_escape_string`
- `mysql_query`
- `mysql_real_query`
- `CLIENT_MULTI_STATEMENTS`

## Preserved

- official Protobuf
- Main/Sub Reactor ownership
- Redis Presence/unread
- SQLite local history
- groups/friends/messages
- online/offline message delivery
- online/offline file delivery
- SHA-256 final integrity checks

## Not yet added

- mTLS/client certificates
- MySQL connection pool
- asynchronous MySQL worker pool
- automatic stale upload checkpoint garbage collection

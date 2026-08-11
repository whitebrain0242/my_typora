# Protobuf Guide — v8.2

从 v8.2 开始，本项目只使用官方 Protobuf C++ toolchain。

请阅读：

```text
OFFICIAL_PROTOBUF_GUIDE.md
```

需要：

```text
protobuf-compiler
libprotobuf-dev
```

构建：

```text
.proto
→ protoc
→ generated .pb.h/.pb.cc
→ libprotobuf
```

不存在自定义 wire encoder/decoder。

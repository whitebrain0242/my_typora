# 群组功能使用说明

## 创建

```text
CREATE_GROUP cpp_study
```

创建者自动成为：

```text
owner
```

## 申请加入

```text
APPLY_GROUP cpp_study
```

群主和管理员在线时会立即看到通知。

离线时申请仍然保存在：

```text
group_join_requests
```

管理员下次登录会看到待处理数量提示。

## 查看申请

```text
GROUP_REQUESTS cpp_study
```

仅：

```text
owner / admin
```

可执行。

## 批准

```text
APPROVE_GROUP cpp_study bob
```

## 拒绝

```text
REJECT_GROUP cpp_study bob
```

## 查看自己的群

```text
MY_GROUPS
```

示例：

```text
[system] joined groups (2):
  cpp_study [owner] owner=alice
  linux_group [member] owner=bob
```

## 查看成员

```text
GROUP_MEMBERS cpp_study
```

显示角色和在线状态。

## 管理员

群主添加：

```text
ADD_GROUP_ADMIN cpp_study bob
```

群主取消：

```text
REMOVE_GROUP_ADMIN cpp_study bob
```

## 移除成员

```text
REMOVE_GROUP_MEMBER cpp_study charlie
```

权限：

```text
owner: 可以移除 admin/member
admin: 只能移除普通 member
```

## 主动退出

```text
LEAVE_GROUP cpp_study
```

群主不能直接退出，需要：

```text
DISSOLVE_GROUP cpp_study
```

## 群聊

```text
GROUP_MSG cpp_study hello everyone
```

在线成员立即收到。

离线成员在登录后收到：

```text
[offline #G15] [group cpp_study] [alice] hello everyone
```

## 群历史

```text
HISTORY_GROUP cpp_study 20
```

只有当前仍属于群组的成员能查看。

# Git

## 一、初始化与配置（第一次用 Git）

bash

config—— configuration配置、设置、配置文件

git config —— **配置 Git 的个人信息、全局参数**



```
# 1. 初始化一个本地Git仓库（在项目文件夹里执行）
git init

# 2. 配置用户名（提交代码时显示的名字）
git config --global user.name "你的名字"

# 3. 配置邮箱（提交代码时显示的邮箱）
git config --global user.email "你的邮箱"

# 查看当前配置
git config --list
```

## 二、查看状态与日志

bash







```
# 查看文件修改状态（新增/修改/删除）
git status

# 查看提交历史记录
git log

# 简洁版日志（推荐）
git log --oneline
```

## 三、文件提交（本地操作）

bash







```
# 1. 添加单个文件到暂存区
git add 文件名

# 2. 添加所有修改文件到暂存区（最常用）
git add .

# 3. 提交到本地仓库（必须写备注）//m就是message，提交备注，信息
git commit -m "这里写本次修改的说明"

# 4. 一步完成 add + commit（仅限已追踪文件）
git commit -am "修改说明"
```

## 四、分支操作（开发必备）

bash







```
# 查看所有本地分支
git branch

# 查看所有分支（本地+远程）
git branch -a

# 创建新分支
git branch 分支名

# 切换分支
git checkout 分支名

# 创建并直接切换到新分支（最常用）
git checkout -b 分支名

# 合并指定分支到当前分支
git merge 要合并的分支名

# 删除本地分支
git branch -d 分支名
```

## 五、远程仓库（GitHub/Gitee/GitLab）

bash







```
# 关联远程仓库
git remote add origin 远程仓库地址

# 推送到远程仓库（第一次推送）
git push -u origin 主分支名（通常是main/master）

# 后续常规推送
git push

# 拉取远程最新代码
git pull

# 查看远程仓库地址
git remote -v
```

## 六、撤销与回退

bash







```
# 撤销工作区修改（未add之前）
git checkout -- 文件名

# 撤销暂存区文件（已add未commit）
git reset HEAD 文件名

# 回退到上一个提交版本
git reset --hard HEAD^
```
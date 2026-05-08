# Git 协作工作流：Fork → 本地开发 → Pull Request

本文档面向团队成员，介绍如何从实验室 GitHub 仓库 fork 代码、在本地 VSCode 开发、保持同步、并提交 Pull Request 的标准流程。

## 1. 一次性配置

### 1.1 Fork 实验室仓库

1. 在浏览器打开实验室 GitHub 仓库页面
2. 点击右上角 **Fork** 按钮
3. Owner 选择你自己的 GitHub 账号
4. 点击 **Create fork**

完成后你会在 `https://github.com/<你的用户名>/<仓库名>` 看到自己的 fork。

### 1.2 克隆到本地

```bash
cd ~/work
git clone git@github.com:<你的用户名>/<仓库名>.git
cd <仓库名>
```

> 如果还没配置 SSH Key，先用 `git clone https://github.com/<你的用户名>/<仓库名>.git`

### 1.3 添加 upstream 远程

```bash
git remote add upstream git@github.com:<实验室组织>/<仓库名>.git
git remote -v
```

输出应显示两个远程：

```
origin    git@github.com:<你的用户名>/<仓库名>.git (fetch)
origin    git@github.com:<你的用户名>/<仓库名>.git (push)
upstream  git@github.com:<实验室组织>/<仓库名>.git (fetch)
upstream  git@github.com:<实验室组织>/<仓库名>.git (push)
```

- `origin` 指向你自己的 fork，你有推送权限
- `upstream` 指向实验室仓库，拉取最新代码用

### 1.4 VSCode 配置（推荐）

- 安装 GitLens 和 GitHub Pull Requests 扩展
- 在 VSCode 左下角确认当前分支显示正确
- 在 Settings 中启用 `git.autofetch`，VSCode 会定时自动拉取远程更新

## 2. 日常开发工作流

### 2.1 同步上游最新代码

每次开始新功能前，先把实验室仓库的最新代码拉到本地：

```bash
git checkout main
git fetch upstream
git merge upstream/main
git push origin main
```

> 如果团队使用 `master` 分支，把命令中的 `main` 替换为 `master`。

### 2.2 创建功能分支

**永远不要在 main 分支上直接修改。** 每项改动都从 main 拉出新分支：

```bash
git checkout main
git checkout -b feature/<简短描述>
```

命名示例：`feature/formation-velocity-fix`、`fix/sitl-thread-timeout`、`doc/update-readme`

### 2.3 开发 & 提交

在 VSCode 中进行代码修改，然后用 Source Control 面板（`Ctrl+Shift+G`）或命令行提交：

```bash
git add <改动的文件>
git commit -m "fix: 修复编队节点速度兜底逻辑"
```

**Commit message 规范**（参考 [Conventional Commits](https://www.conventionalcommits.org/)）：

| 前缀 | 用途 |
|------|------|
| `feat:` | 新功能 |
| `fix:` | 修复 bug |
| `doc:` | 文档变更 |
| `refactor:` | 重构（不改变功能） |
| `test:` | 测试相关 |
| `chore:` | 构建/工具/依赖 |

### 2.4 推送到自己的 fork

```bash
git push origin feature/<简短描述>
```

### 2.5 发起 Pull Request

1. 打开你自己的 fork 页面，GitHub 会提示 **Compare & pull request**，点击它
2. 确认 base repository 是**实验室仓库**，head repository 是你的 fork
3. 填写 PR 描述：
   - 说明做了什么改动
   - 为什么这么改
   - 如何测试
4. 点击 **Create pull request**

## 3. 同步上游（上游有新提交后）

当实验室仓库有他人合并的 PR 后，你需要同步到自己的 fork：

```bash
git checkout main
git fetch upstream
git merge upstream/main
git push origin main
```

然后把最新 main 合并到你的功能分支：

```bash
git checkout feature/<你的分支>
git merge main
```

有冲突时在 VSCode 中解决。VSCode 会用颜色标记冲突区域，点击 `Accept Current` / `Accept Incoming` / `Accept Both` 即可选择保留哪边。

## 4. 处理 PR Review 反馈

1. 在本地功能分支上修改代码
2. `git add` / `git commit` 提交新的改动
3. `git push origin feature/<你的分支>` 推送
4. PR 会自动更新，不需要关闭重建

> **不要**因为 review 意见而 `git push --force`，除非已经和 Reviewer 确认过。

## 5. 完整流程速查

```bash
# 1. 同步上游
git checkout main
git fetch upstream
git merge upstream/main
git push origin main

# 2. 开分支
git checkout -b feature/my-change

# 3. 改代码 → 提交 → 推送
git add .
git commit -m "fix: 描述你的改动"
git push origin feature/my-change

# 4. 去 GitHub 页面创建 PR

# 5. 等 review → 修改 → 再 push → PR 自动更新
```

## 6. 常见问题

- **Q: PR 合入后要删分支吗？**
  A: 在 GitHub PR 页面点 "Delete branch" 按钮即可。本地用 `git branch -d feature/xxx` 清理，然后 `git fetch upstream --prune` 清理远程引用。

- **Q: 怎么在命令行创建 PR？**
  A: 安装 [GitHub CLI](https://cli.github.com/) 后使用 `gh pr create`。

- **Q: PR 改错分支了怎么办？**
  A: 如果 PR 还没合入，可以在 GitHub PR 页面点 "Edit" 修改 base 分支。如果已经合入，联系管理员 revert。

- **Q: 多人同时改同一个文件冲突怎么办？**
  A: `git merge main` 后在 VSCode 中逐处解决冲突，保证编译/运行通过后再 push。

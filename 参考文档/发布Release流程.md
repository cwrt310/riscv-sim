# 发布 Release 流程

> 日期：2026-09-05 ｜ 适用：riscv-sim 项目（4 人团队，谁发版看这份）
> 配套：仓库根的 `make_release.sh` 一键脚本；踩坑记录 13/15 号坑是本流程的由来。

## 一、什么时候要发 Release

- 代码有**大改动**（新功能、重要修复），要给队友/老师一个**双击就能跑**的版本；
- 不用每次 commit 都发——发版要花 5 分钟，攒一批值得炫耀的功能再发。

## 二、一键发布（推荐，三条命令以内）

**首次使用前**：在 Qt Creator 左下角切到 Release 构建一次（让 Release 构建目录生成出来），以后就不用管了。

```bash
cd ~/Desktop/riscv-sim        # 进仓库
./make_release.sh V2.1        # 版本号换成这次的
```

脚本自动完成三步（每步都有 ✓ 输出，任何一步失败会立即停止）：

| 步 | 做什么 |
|---|---|
| ① | 重新编译 Release（**防止发出旧 exe**，见坑 13） |
| ② | `windeployqt` 把 Qt DLL 拷到 exe 旁边（没装 Qt 的电脑也能跑） |
| ③ | 打 zip 到桌面：`riscv-sim-V2.1-win64.zip`（只含运行必需文件） |

**Qt 不在 E 盘（换电脑/队友环境）**：不用改脚本，用环境变量覆盖：

```bash
QT_ROOT=/c/Qt/6.11.2/mingw_64 ./make_release.sh V2.1
```

全部可覆盖变量：`QT_ROOT`、`MINGW_ROOT`、`CMAKE_BIN`、`NINJA_BIN`、`TAR`、`DESKTOP`（含义见脚本顶部注释）。

## 三、网页最后一步（必须亲手点，用 GitHub 账号）

脚本跑完会打印这 6 步，这里是详细版：

1. 打开 `https://github.com/cwrt310/riscv-sim/releases`
2. 点 **Draft a new release**
3. **Choose a tag** 输入框填版本号 `V2.1` → 下拉里选 **"Create new tag on publish"**（tag 用过的不能重复，重复会提示已存在）
4. **Release title** 填 `V2.1 <一句话卖点>`，比如 `V2.1 数据通路可视化版`
5. **Describe** 写更新内容（模板见下一节）
6. 把桌面上的 `riscv-sim-V2.1-win64.zip` **拖进** "Attach binaries" 区域 → 点 **Publish release**

发布完成后任何人打开仓库页面就能看到 Release 卡片 + 下载 zip。

### 更新说明模板

```markdown
## 新功能

- ...

## 修复

- ...

## 文档与测试

- ...
```

## 四、手动流程（脚本背后的原理，排查用）

```bash
# ① 编译 Release（Qt Creator 左下角切 Release 点构建也行）
cmake --build build/Desktop_Qt_6_11_2_MinGW_64_bit_Release

# ② windeployqt：拷 Qt DLL
cd build/Desktop_Qt_6_11_2_MinGW_64_bit_Release
"/e/qt/6.11.2/mingw_64/bin/windeployqt.exe" --no-translations riscv-sim.exe

# ③ 打 zip（Windows 自带 tar = bsdtar，支持 zip；Git Bash 的 tar 不支持！）
/c/Windows/System32/tar.exe -a -c -f ~/Desktop/riscv-sim-V2.1-win64.zip \
    riscv-sim.exe *.dll platforms styles imageformats iconengines networkinformation generic
```

## 五、常见坑速查

| 坑 | 怎么发现 / 怎么办 |
|---|---|
| **发出的 exe 是旧代码**（坑 13） | Release 不会自动重编。对比时间戳：`ls -la build/..._Release/riscv-sim.exe mainwindow.cpp`；用 `make_release.sh` 强制先编译 |
| **tag 已存在** | GitHub 会拒绝。换个版本号，或先确认上一个发到哪了：`git tag -l` / `git ls-remote --tags origin` |
| **zip 误提交进仓库**（坑 12） | zip 必须放桌面等仓库外位置，别放仓库根目录；`.gitignore` 已有 `*.zip` |
| **windeployqt 报 dxcompiler.dll 警告** | 无害（不用 Direct3D 12 就没事），忽略 |
| **Git Bash 的 tar 打不了 zip** | 那是 GNU tar。要用 Windows 自带的 `/c/Windows/System32/tar.exe`（脚本里已处理） |
| **脚本报"找不到 Release 构建目录"** | 先在 Qt Creator 左下角切 Release 构建一次 |

## 六、版本号约定

| 项 | 规则 | 例子 |
|---|---|---|
| tag | `V` + 主.次；大功能 +1 主版本，小修 +0.1 | V1.0 → V2.0 → V2.1 |
| zip 名 | `riscv-sim-<tag>-win64.zip` | riscv-sim-V2.1-win64.zip |
| 检查已有 tag | `git tag -l`（本地）/ `git ls-remote --tags origin`（远程） | |

## 七、发布档案

| Release | Tag | 指向 commit | 内容 |
|---|---|---|---|
| 1 | `V1.0` | `c4822eb` | 13 条指令 + 基础 Qt 界面 |
| 2 | `V2.0` | （发布后补） | 数据通路可视化 + 灯效 + 速度输入框 + 测试手册 |

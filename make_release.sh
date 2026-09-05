#!/usr/bin/env bash
# =============================================================
# 一键发布脚本：编译 Release → windeployqt 打包 → 压 zip 到桌面
#
# 用法：./make_release.sh <版本号>
# 示例：./make_release.sh V2.1
#
# 前置：第一次用之前，先在 Qt Creator 里切到 Release 构建一次
#       （左下角切换器），让构建目录生成出来。
# =============================================================
set -e   # 任何一步失败立即停止

# ---------- 工具链路径 ----------
# 默认按这台电脑（Qt 装 E 盘）配好；换电脑不用改脚本，
# 跑的时候用环境变量覆盖即可，例如：
#   QT_ROOT=/c/Qt/6.11.2/mingw_64 ./make_release.sh V2.1
QT_DIR="${QT_ROOT:-/e/qt/6.11.2/mingw_64}"     # Qt 安装路径
MINGW_DIR="${MINGW_ROOT:-/e/qt/Tools/mingw1310_64}"
CMAKE_DIR="${CMAKE_BIN:-/e/qt/Tools/CMake_64/bin}"
NINJA_DIR="${NINJA_BIN:-/e/qt/Tools/Ninja}"
TAR="${TAR:-/c/Windows/System32/tar.exe}"      # Windows 自带 bsdtar（能打 zip）
ZIP_OUT_DIR="${DESKTOP:-$USERPROFILE/Desktop}" # zip 输出到桌面

# ---------- 版本号检查 ----------
VERSION="$1"
if [ -z "$VERSION" ]; then
    echo "用法：./make_release.sh <版本号>"
    echo "示例：./make_release.sh V2.1"
    exit 1
fi

# ---------- 0. 定位仓库根目录（脚本放在仓库根，从哪跑都行） ----------
cd "$(dirname "$0")"
REPO_ROOT="$(pwd)"
BUILD_DIR="$REPO_ROOT/build/Desktop_Qt_6_11_2_MinGW_64_bit_Release"
ZIP_OUT="$ZIP_OUT_DIR/riscv-sim-$VERSION-win64.zip"

# ---------- 1. 准备工具链 ----------
export PATH="$QT_DIR/bin:$MINGW_DIR/bin:$CMAKE_DIR:$NINJA_DIR:$PATH"

# ---------- 2. 编译 Release ----------
if [ ! -d "$BUILD_DIR" ]; then
    echo "✗ 找不到 Release 构建目录："
    echo "  $BUILD_DIR"
    echo "  请先在 Qt Creator 左下角切到 Release 构建一次。"
    exit 1
fi
echo "== ① 编译 Release =="
cmake --build "$BUILD_DIR"
echo "✓ 编译完成"

# ---------- 3. windeployqt：把 Qt DLL 拷到 exe 旁边 ----------
echo "== ② windeployqt 打包 Qt DLL =="
cd "$BUILD_DIR"
"$QT_DIR/bin/windeployqt.exe" --no-translations riscv-sim.exe > /dev/null
echo "✓ DLL 打包完成"

# ---------- 4. 打 zip（只挑运行必需文件，不带 CMake 构建垃圾） ----------
echo "== ③ 打包 zip =="
"$TAR" -a -c -f "$ZIP_OUT" \
    riscv-sim.exe *.dll platforms styles imageformats iconengines networkinformation generic
echo "✓ zip 已生成：$ZIP_OUT"

# ---------- 5. 提醒最后一步 ----------
echo ""
echo "=============================================="
echo "  还剩最后一步（必须亲手点，用你的 GitHub 账号）："
echo "  1. 打开 https://github.com/cwrt310/riscv-sim/releases"
echo "  2. 点 Draft a new release"
echo "  3. Choose a tag 输入 $VERSION → 选 Create new tag on publish"
echo "  4. 标题填版本号，描述写更新内容"
echo "  5. 把桌面上的 zip 拖进 Attach binaries"
echo "  6. 点 Publish release"
echo "=============================================="

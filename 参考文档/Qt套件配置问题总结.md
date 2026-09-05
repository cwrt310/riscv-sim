# Qt 套件（Kit）配置问题总结

> 背景：队友新装 Qt（装在 `D:\QT`），新建项目时找不到 MinGW 套件，套件页一片红/黄图标。本文记录这一连串问题与正确配法，全队通用。
> 更新日期：2026-09-03

---

## 〇、一句话理解 Kit

**Kit = 编译器 + Qt 版本 + 调试器 + CMake 四件套**。任何一件缺失或不匹配，图标就红（坏了）或黄（不完整）。

---

## 一、问题连串（按发现顺序）

| # | 现象 | 原因 | 解决 |
|---|---|---|---|
| 1 | 编译器页：MinGW 条目无效 | 把**调试器 `gdb.exe`** 填进了 C/C++ 编译器两栏 | 改成 `gcc.exe` / `g++.exe` |
| 2 | Qt 版本页报红：*No compiler can produce code for this Qt version* | 编译器 ABI 是 msvc2022，和 Qt 要的 msys/mingw 不匹配 | 见 #5 |
| 3 | 套件里 CMake Tool = None | CMake 要在左侧单独的「CMake」页添加，不会自动有 | 添加 `D:\QT\Tools\CMake_64\bin\cmake.exe` |
| 4 | 套件的 Qt 版本 = 「无」 | 没选 | 下拉选 `Qt 6.11.2 (mingw_64)` |
| 5 | 都选了还是黄 | `Provide manually` 勾着，ABI 仍手动指成 **msvc2022** | 去掉勾让 ABI 自动识别，或手动改 mingw/msys |
| 附带 | 自动生成的 MSVC kit 带黄叹号 | 自动生成的 MSVC kit 没有匹配的 Qt | **无害，忽略**；把 MinGW kit 设为默认即可 |

> 连锁关系：#1 编译器填错 → #2 Qt 版本报红 → 套件全红 → 新建项目找不到可用套件。

---

## 二、正确配置清单（四件套）

| 部件 | 路径（队友机 `D:\QT`） |
|---|---|
| C 编译器 | `D:\QT\Tools\mingw1310_64\bin\gcc.exe` |
| C++ 编译器 | `D:\QT\Tools\mingw1310_64\bin\g++.exe` |
| 调试器 | `D:\QT\Tools\mingw1310_64\bin\gdb.exe` |
| Qt 版本 | `D:\QT\6.11.2\mingw_64\bin\qmake.exe` |
| CMake | `D:\QT\Tools\CMake_64\bin\cmake.exe` |

（我的机器装 E 盘：`E:\qt\...`，结构相同。）

配好后 kit 的 ABI 应为 `x86-windows-msys(mingw)-pe-64bit`，与 Qt 版本一致 → 图标正常 → **Make Default**。

---

## 三、三条口诀

1. **gdb ≠ gcc**：gdb 是调试器，gcc/g++ 才是编译器，别填串；
2. **同族配对**：mingw 的 Qt 配 mingw 编译器（gcc/g++），别配 MSVC；
3. **别手动指 ABI**：不勾 `Provide manually`，让 Qt Creator 自己问编译器。

---

## 四、红 vs 黄 怎么认

| 图标 | 含义 | 典型原因 |
|---|---|---|
| ❌ 红 | 坏了 | 路径无效（如编译器填 gdb）、严重不匹配 |
| ⚠️ 黄 | 不完整 | 缺 Qt 版本、缺 CMake、ABI 不匹配 |
| 正常 | OK | 四件套齐且同族 |

---

## 五、名词表（这一连串里遇到的）

### 环境配置类

| 名词 | 一句话解释 |
|---|---|
| **Kit（构建套件）** | 「编译器 + Qt 版本 + 调试器 + CMake」打包成的一套，新建项目时选它 |
| **MinGW** | GCC 编译器移植到 Windows 的版本，我们用的编译器族（gcc/g++） |
| **MSVC** | 微软的编译器（Visual Studio 自带），和 MinGW 不同族，不能混配 |
| **gcc / g++** | MinGW 的 C / C++ 编译器，真正干编译活的 |
| **gdb** | MinGW 的调试器（断点/单步），**不是编译器**，别填串 |
| **CDB** | Windows/MSVC 的调试器，自动检测到但我们不用 |
| **ABI** | 二进制的「方言」（谁编译的、多少位）；编译器和 Qt 的 ABI 必须一致才能配对 |
| **qmake** | Qt 库的「身份证」程序，Qt Creator 靠 `qmake.exe` 认出整套 Qt |
| **CMake** | 构建系统，读 `CMakeLists.txt` 驱动编译链接 |
| **CMake generator** | 告诉 CMake 生成哪种构建脚本，我们选 `MinGW Makefiles` |
| **Provide manually** | 「手动指定 ABI」；勾了就不自动识别——这次踩坑的根源 |
| **Make Default** | 把某个 kit 设为默认 |
| **Debug / Release** | 两种构建配置：带调试信息 / 优化加速；同一套 kit 两种编法 |
| **windeployqt** | 把 exe 依赖的 Qt DLL 拷到旁边的工具，打包分发用 |
| **Target triple / Sysroot** | 交叉编译术语，桌面开发用不到，忽略 |

### 代码类

| 名词 | 一句话解释 |
|---|---|
| **QApplication** | 程序总管家，每个 Qt 程序第一句，负责事件循环 |
| **QWidget** | 窗口/控件的祖宗，所有控件都继承它 |
| **信号槽（connect）** | 「按钮被点 → 执行某段代码」的机制 |
| **布局（QV/HBoxLayout）** | 自动排列控件，不用算坐标 |
| **lambda `[&]`** | 就地定义的函数，能借外面变量，用来写按钮逻辑 |
| **QPainter / paintEvent** | 画笔 / 「重绘时自动被调用」的函数，自绘图形用 |
| **.ui / Designer** | Qt 拖拽设计界面的工具；我们走纯代码路线，不用它 |

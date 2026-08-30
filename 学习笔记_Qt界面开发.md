# 学习笔记：Qt 界面开发

> 日期：2026-08-30 ｜ 主题：从零把命令行程序升级成 Qt 图形界面（riscv-sim 实战）

---

## 一、Qt 和命令行的根本区别

| | 命令行 | Qt 界面 |
|---|---|---|
| 流程 | 顺序跑完就退出 | 停在 `app.exec()` 等人操作 |
| 谁驱动 | 程序推着你走 | 你点一下，程序动一下 |
| 结束 | `return 0` | 关窗口才返回 |

Qt 的本质是「**事件循环**」：`app.exec()` 让程序停在那里，等用户点按钮、敲键盘，收到一个「事件」就调用对应的处理函数。**命令行是顺序，Qt 是响应**——这是最大的心智转变。

---

## 二、Qt 五个核心概念

1. **QApplication**：程序总管家，每个 Qt 程序第一句，负责收鼠标键盘再分发给控件；
2. **QWidget**：窗口（也是所有控件的「祖宗」，按钮/表格/文本框都继承它）；
3. **控件 = `new` + 爸爸**：`new QPushButton("运行", &window)`，第二个参数是父对象，爸爸销毁时自动删儿子，**永远不用手动 `delete`**；
4. **布局 Layout**：`QVBoxLayout`（竖排）/ `QHBoxLayout`（横排），`addWidget`/`addLayout` 排控件，可以嵌套（大竖盒里塞小横盒）；
5. **信号槽 Signal & Slot**：`connect(谁, 什么信号, 干什么)`，Qt 的灵魂。

> 一句话套路：`new` 出控件 → 塞进 Layout 排好 → `connect` 连上「点按钮干什么」→ `app.exec()` 等人操作。

---

## 三、lambda（`[&]() {...}`）

lambda = 就地定义、存进变量的匿名函数，用来「把重复代码打包反复调用」：

```cpp
auto refresh = [&]() {          // [&] = 借外面的变量进来
    regTable->item(i, 1)->setText(...);  // 能读写外面的 regTable
};
refresh();   // 调用
```

- **`[&]` 捕获列表**：让 lambda 里能「看见并修改」外面的变量（`cpu`、`regTable` 这些）；
- 没有 `[&]`，lambda 摸不到外面的变量，也就没法刷新表格。

---

## 四、常用控件速查

| 控件 | 用途 | 关键方法 |
|---|---|---|
| QPushButton | 按钮 | `clicked` 信号 |
| QLabel | 文字标签 | `setText()` |
| QPlainTextEdit | 多行输入（汇编） | `toPlainText()` 读、`setPlainText()` 写、`setPlaceholderText()` |
| QTextEdit | 多行显示（日志） | `append()`、`setReadOnly(true)` |
| QTableWidget | 表格（寄存器/内存） | `setRowCount()`、`setItem()`、`setHorizontalHeaderLabels()` |

---

## 五、读文件三件套

```cpp
QString path = QFileDialog::getOpenFileName(&window, "标题", "", "文本 (*.txt *.asm)");
//  ① 弹文件框选文件，返回路径；点「取消」返回空字符串

QFile file(path);
file.open(QIODevice::ReadOnly | QIODevice::Text);   // ② 打开文件（只读 + 文本）

QTextStream in(&file);
codeEdit->setPlainText(in.readAll());               // ③ 读全部内容
```

记一个链条：**`QFileDialog` 选路径 → `QFile` 打开 → `QTextStream` 读**。写文件就是反过来（`WriteOnly` + `<<`）。

---

## 六、弹窗 QMessageBox

```cpp
QMessageBox::information(&window, "标题", "内容文字");
```

三个参数：父窗口、标题、正文（`\n` 换行）。变体：`warning`（警告）、`question`（问是/否），套路一样。

---

## 七、打包成独立 exe

1. Qt Creator 左下角切 **Release** 编译（默认只有 Debug，要在「项目」面板勾选 Release）；
2. 用 `windeployqt` 把 exe 依赖的 Qt DLL 拷到它旁边：

```bash
cd /c/Users/cwrt/Desktop/riscv-sim/build/Desktop_Qt_6_11_2_MinGW_64_bit_Release
"/e/qt/6.11.2/mingw_64/bin/windeployqt.exe" riscv-sim.exe
```

3. 整个 Release 目录（exe + Qt DLL + `platforms/`）即可双击运行、分发到没装 Qt 的电脑。

---

## 八、踩过的坑

详见《开发踩坑记录.md》，这里列个索引：

1. 单步无边界检查 → 执行完继续单步越界读垃圾指令崩溃（最经典）；
2. 内存表固定 16 行 → 改成 `setRowCount` 动态；
3. 构建配置只有 Debug → 项目面板勾选 Release。

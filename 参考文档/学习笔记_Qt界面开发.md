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
| QDoubleSpinBox | 带小数的数字输入框 | `setRange()`、`setSuffix(" 条/秒")`、`valueChanged` 信号 |
| QTimer | 定时器（驱动动画） | `setInterval(ms)`、`start()`/`stop()`、`timeout` 信号 |

---

## 五、⭐ update() vs repaint()：动画做不出来八成栽在这

这是本项目最有价值的一课（详见《调试方法论_一个bug的完整复盘.md》）。

| | `update()` | `repaint()` |
|---|---|---|
| 语义 | **排队**：往事件队列塞一张重绘申请单 | **同步**：立刻调 `paintEvent()`，画完才返回 |
| 连续调用 | 会被 Qt **合并成一次** | 每次都真画 |
| 适合 | 普通状态变化（改完等 Qt 找空闲画） | **每一帧都必须看见**的动画/单步演示 |

Qt 官方文档原话：

> `void QWidget::update()` — This function does not cause an immediate repaint; instead it **schedules** a paint event for processing when Qt returns to the main event loop.

**踩坑现场**：QTimer 每 500ms 走一条指令并 `update()` 数据通路图，结果 8 条指令只画了 1 次——8 张申请单被合并了，`paintEvent()` 读到的是最后一次的状态。

```cpp
// ✗ 快速连续改状态时，中间帧全丢
void refreshUI() { datapath->setHighlightMask(mask); }   // 内部只调 update()

// ✓ 每一拍都强制落屏
void refreshUI() { datapath->setHighlightMask(mask); datapath->repaint(); }
```

**推论（很重要）**：`update()` 是异步的，所以**「改了状态」和「画出来」之间隔着事件循环**。事件循环越忙，中间帧丢得越多——所以还要避免在高频刷新的函数里做重活：

```cpp
// ✗ 每拍 new 几十个对象，事件循环被占满
memTable->setItem(i, 1, new QTableWidgetItem(v));
// ✓ 复用已有单元格
if (auto *it = memTable->item(i, 1)) it->setText(v);
else memTable->setItem(i, 1, new QTableWidgetItem(v));
```

---

## 六、⭐ 用 QTimer 做「能看见过程」的执行

**问题**：后端 `cpu.run()` 是个 `for` 循环，几微秒跑完。在循环里 `refreshUI()` 也没用——**整个循环期间事件循环被阻塞，一帧都画不出来**，还会让窗口显示"未响应"。

**解法**：不要在循环里跑，**把循环拆给 QTimer 一拍一条**：

```cpp
// 1. 建 timer，连到"走一步"的槽
runTimer = new QTimer(this);
connect(runTimer, &QTimer::timeout, this, &MainWindow::onTimerTick);

// 2. 点「运行」= 启动 timer（立刻返回，事件循环继续转）
void MainWindow::onRunClicked() {
    runTimer->setInterval(int(1000.0 / speedBox->value()));
    runTimer->start();
}

// 3. 每拍只做一条，做完就交还控制权给事件循环
void MainWindow::onTimerTick() {
    if (cpu.finished()) { runTimer->stop(); /* 恢复按钮 */ return; }
    cpu.step();
    refreshUI();     // 里面有 repaint()
}
```

三个要点：

1. **`start()` 立刻返回**，不阻塞。界面全程可响应，「暂停」按钮才点得动（`runTimer->stop()`）；
2. **状态要存成员变量**（`tickCount`、`isRunning`），不能用局部变量——每拍是**独立的函数调用**，栈上的东西不留存；
3. **速度 = `setInterval(ms)`**。给用户的输入框用「条/秒」，内部 `1000.0 / ips` 换算。

> 一句话：**Qt 里"随时间推进的过程"不用 for 循环写，用 QTimer 把循环体拆成一拍一次。**

---

## 七、自定义控件画图（QPainter）

继承 `QWidget` 重写 `paintEvent()` 就能自己画（数据通路图就是这么来的）：

```cpp
class DatapathWidget : public QWidget {
    Q_OBJECT
public:
    void setHighlightMask(uint32_t m) { m_mask = m; update(); }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);   // 抗锯齿，线条不毛糙
        p.setBrush(QColor(220,235,250));           // 填充色
        p.setPen(QPen(Qt::black, 2));              // 线条色 + 粗细
        p.drawRect(x, y, w, h);
        p.drawLine(x1, y1, x2, y2);
        p.drawPolygon(arrow);                      // 画箭头三角
        p.drawText(x, y, "PC");
    }
private:
    uint32_t m_mask = 0;   // 画什么由成员变量决定，外部改它 + 重绘
};
```

套路要记住：**`paintEvent()` 只负责「照着成员变量画」，绝不改状态**。外部改成员变量 → 触发重绘 → `paintEvent()` 读新值。

几个实用技巧：

| 需求 | 做法 |
|---|---|
| 文字居中 | `x + (w - QFontMetrics(p.font()).horizontalAdvance(s)) / 2` |
| 虚线 | `QPen(color, 2, Qt::DashLine)` |
| 高亮/常态两套样式 | 一个 `bool highlight` 参数，选不同的画笔和填充 |
| 布局随窗口自适应 | 用 `width()` 算盒宽：`qBound(80, (avail - 间距总和)/n, 130)` |
| 避免重复代码 | 用 lambda 打包 `drawBox`/`drawArrowH`/`drawArrowV` |

---

## 八、读文件三件套

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

## 九、弹窗 QMessageBox

```cpp
QMessageBox::information(&window, "标题", "内容文字");
```

三个参数：父窗口、标题、正文（`\n` 换行）。变体：`warning`（警告）、`question`（问是/否），套路一样。

---

## 十、打包成独立 exe

1. Qt Creator 左下角切 **Release** 编译（默认只有 Debug，要在「项目」面板勾选 Release）；
2. 用 `windeployqt` 把 exe 依赖的 Qt DLL 拷到它旁边：

```bash
cd /c/Users/cwrt/Desktop/riscv-sim/build/Desktop_Qt_6_11_2_MinGW_64_bit_Release
"/e/qt/6.11.2/mingw_64/bin/windeployqt.exe" riscv-sim.exe
```

3. 整个 Release 目录（exe + Qt DLL + `platforms/`）即可双击运行、分发到没装 Qt 的电脑。

---

## 十一、踩过的坑

详见《开发踩坑记录.md》，这里列个索引：

1. **`update()` 被合并导致动画只剩最后一帧** → 改 `repaint()`（最有价值的一课，见第五节）；
2. 单步无边界检查 → 执行完继续单步越界读垃圾指令崩溃（最经典）；
3. 未实现指令 throw 穿透槽函数 → 崩溃，槽里必须 `try-catch`；
4. 内存表固定 16 行 → 改成 `setRowCount` 动态；
5. 构建配置只有 Debug → 项目面板勾选 Release。

---

## 十二、一页速查

```cpp
// —— 建控件 ——
auto *btn = new QPushButton("运行");
auto *box = new QDoubleSpinBox;  box->setRange(0.2, 50);  box->setSuffix(" 条/秒");

// —— 排布局 ——
auto *row = new QHBoxLayout;  row->addWidget(btn);  row->addStretch();
auto *col = new QVBoxLayout;  col->addLayout(row);  col->addWidget(table, 1);  // 1=可拉伸
auto *central = new QWidget(this);  central->setLayout(col);  setCentralWidget(central);

// —— 连信号 ——
connect(btn, &QPushButton::clicked, this, &MainWindow::onRun);            // 连成员函数
connect(box, &QDoubleSpinBox::valueChanged, this, [this](double v){...}); // 连 lambda

// —— 定时驱动 ——
timer = new QTimer(this);  timer->setInterval(500);
connect(timer, &QTimer::timeout, this, &MainWindow::onTick);
timer->start();   /* ... */   timer->stop();

// —— 自绘 ——
void paintEvent(QPaintEvent*) override { QPainter p(this); /* 照成员变量画 */ }
widget->repaint();   // 需要立刻看见时用它，不是 update()
```


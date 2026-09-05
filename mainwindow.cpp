#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QPainter>
#include <QTimer>
#include <QHeaderView>

//DatapathWidget 实现

DatapathWidget::DatapathWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(210);
    setMinimumWidth(900);
}

void DatapathWidget::setHighlightMask(uint32_t mask)
{
    m_mask = mask;
    update();
}

void DatapathWidget::setInstText(const QString& text)
{
    m_instText = text;
    update();
}

void DatapathWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QColor normalColor(220, 235, 250);
    QColor borderColor(80, 80, 80);
    QColor textColor(30, 30, 30);
    QColor lineColor(120, 120, 120);
    QColor highlightColor(255, 200, 100);

    // 6 个部件排成一行，宽度按窗口自适应
    const int n = 6;
    int marginL = 14, marginR = 14;
    int avail = width() - marginL - marginR;
    int boxW = qBound(80, (avail - (n - 1) * 26) / n, 130);
    int spacing = (n > 1) ? (avail - boxW * n) / (n - 1) : 0;
    if (spacing < 18) spacing = 18;
    int boxH = 52;
    int col = boxW + spacing;

    int y = 78;                    // 唯一一行的纵坐标
    int midY = y + boxH / 2;

    int xPC  = marginL;
    int xIF  = xPC  + col;
    int xID  = xIF  + col;
    int xREG = xID  + col;
    int xEX  = xREG + col;
    int xMEM = xEX  + col;

    auto isHighlight = [&](uint32_t s) { return (m_mask & s) != 0; };

    auto drawBox = [&](int x, const QString& label, bool highlight) {
        if (highlight) {
            p.setBrush(highlightColor);
            p.setPen(QPen(QColor(200, 120, 30), 3));
        } else {
            p.setBrush(normalColor);
            p.setPen(QPen(borderColor, 2));
        }
        p.drawRect(x, y, boxW, boxH);
        p.setPen(QPen(textColor, 1));
        p.drawText(x + (boxW - QFontMetrics(p.font()).horizontalAdvance(label)) / 2,
                   y + boxH / 2 + 5, label);
    };

    auto drawArrowH = [&](int x1, int x2, int yy, bool highlight) {
        QColor c = highlight ? highlightColor : lineColor;
        int dir = (x2 > x1) ? 1 : -1;
        p.setPen(QPen(c, highlight ? 3 : 2));
        p.drawLine(x1, yy, x2 - dir * 8, yy);
        QPolygon arrow;
        arrow << QPoint(x2, yy)
              << QPoint(x2 - dir * 10, yy - 6)
              << QPoint(x2 - dir * 10, yy + 6);
        p.setBrush(c);
        p.drawPolygon(arrow);
    };

    auto drawArrowV = [&](int x, int yFrom, int yTo, bool highlight) {
        QColor c = highlight ? highlightColor : lineColor;
        int dir = (yTo > yFrom) ? 1 : -1;
        p.setPen(QPen(c, highlight ? 3 : 2));
        p.drawLine(x, yFrom, x, yTo - dir * 8);
        QPolygon arrow;
        arrow << QPoint(x, yTo)
              << QPoint(x - 6, yTo - dir * 10)
              << QPoint(x + 6, yTo - dir * 10);
        p.setBrush(c);
        p.drawPolygon(arrow);
    };

    // 寄存器堆只要被用到（读操作数 或 写回结果）就亮
    bool regOn = isHighlight(STAGE_REG) || isHighlight(STAGE_WB);
    bool wb    = isHighlight(STAGE_WB);
    bool pcWr  = isHighlight(STAGE_PCWR);

    // ===== 顶部提示：现在执行的是第几条、哪条指令 =====
    if (!m_instText.isEmpty()) {
        p.setPen(QPen(QColor(60, 60, 60), 1));
        QFont f = p.font(); f.setBold(true); p.setFont(f);
        p.drawText(marginL, 20, m_instText);
        f.setBold(false); p.setFont(f);
    }

    // ===== 主链路：一行 6 个盒子 =====
    drawBox(xPC,  "PC",         isHighlight(STAGE_PC));
    drawBox(xIF,  "指令存储器", isHighlight(STAGE_IF));
    drawBox(xID,  "译码",       isHighlight(STAGE_ID));
    drawBox(xREG, "寄存器堆",   regOn);
    drawBox(xEX,  "ALU",        isHighlight(STAGE_EX));
    drawBox(xMEM, "数据存储器", isHighlight(STAGE_MEM));

    drawArrowH(xPC  + boxW, xIF,  midY, isHighlight(STAGE_PC)  && isHighlight(STAGE_IF));
    drawArrowH(xIF  + boxW, xID,  midY, isHighlight(STAGE_IF)  && isHighlight(STAGE_ID));
    drawArrowH(xID  + boxW, xREG, midY, isHighlight(STAGE_ID)  && regOn);
    drawArrowH(xREG + boxW, xEX,  midY, regOn                  && isHighlight(STAGE_EX));
    drawArrowH(xEX  + boxW, xMEM, midY, isHighlight(STAGE_EX)  && isHighlight(STAGE_MEM));

    int xPcC  = xPC  + boxW / 2;
    int xIdC  = xID  + boxW / 2;
    int xRegC = xREG + boxW / 2;
    int xExC  = xEX  + boxW / 2;
    int xMemC = xMEM + boxW / 2;

    // ===== 写回总线：走盒子下方，从 ALU / 数据存储器 绕回寄存器堆 =====
    int wbY = y + boxH + 42;
    bool wbFromMem = wb && isHighlight(STAGE_MEM);   // lw：数据从内存来
    bool wbFromAlu = wb && !isHighlight(STAGE_MEM);  // 运算类：数据从 ALU 来

    p.setPen(QPen(wbFromAlu ? highlightColor : lineColor, wbFromAlu ? 3 : 2));
    p.drawLine(xExC, y + boxH, xExC, wbY);           // ALU 下引
    p.setPen(QPen(wbFromMem ? highlightColor : lineColor, wbFromMem ? 3 : 2));
    p.drawLine(xMemC, y + boxH, xMemC, wbY);         // 内存下引
    p.setPen(QPen(wb ? highlightColor : lineColor, wb ? 3 : 2));
    p.drawLine(xMemC, wbY, xRegC, wbY);              // 横向回流
    drawArrowV(xRegC, wbY, y + boxH, wb);            // 向上进寄存器堆
    p.setPen(QPen(QColor(100, 100, 100), 1));
    p.drawText(xRegC + 10, wbY - 7, "写回");

    // ===== 跳转/分支：走盒子上方，从译码/ALU 绕回 PC（虚线）=====
    int pcY = y - 32;
    QColor pcC = pcWr ? highlightColor : lineColor;
    p.setPen(QPen(pcC, pcWr ? 3 : 2, Qt::DashLine));
    p.drawLine(xIdC, y, xIdC, pcY);                  // 译码上引
    p.drawLine(xIdC, pcY, xPcC, pcY);                // 横向回流
    p.drawLine(xPcC, pcY, xPcC, y - 10);
    QPolygon pcArrow;
    pcArrow << QPoint(xPcC, y)
            << QPoint(xPcC - 6, y - 10)
            << QPoint(xPcC + 6, y - 10);
    p.setBrush(pcC);
    p.setPen(Qt::NoPen);
    p.drawPolygon(pcArrow);
    p.setPen(QPen(QColor(100, 100, 100), 1));
    p.drawText(xIdC - 64, pcY - 6, "跳转/分支");

    // ===== 阶段标注 =====
    p.setPen(QPen(QColor(130, 130, 130), 1));
    int tagY = y + boxH + 16;
    auto tag = [&](int x, const QString& s) {
        p.drawText(x + (boxW - QFontMetrics(p.font()).horizontalAdvance(s)) / 2, tagY, s);
    };
    tag(xIF, "IF");
    tag(xID, "ID");
    tag(xREG, "REG");
    tag(xEX, "EX");
    tag(xMEM, "MEM");
}

//MainWindow实现

// 把机器码翻回助记符，给日志和数据通路图上的提示用
static QString describeInst(u32 inst)
{
    if (inst == 0) return "-";
    u32 op     = bits(inst, 6, 0);
    int rd     = int(bits(inst, 11, 7));
    int rs1    = int(bits(inst, 19, 15));
    int rs2    = int(bits(inst, 24, 20));
    int funct3 = int(bits(inst, 14, 12));
    int funct7 = int(bits(inst, 31, 25));

    switch (op) {
    case Op::OP_IMM: {
        static const char* n[8] = {"addi","slli","slti","sltiu","xori","srli","ori","andi"};
        i32 imm = signExtend(bits(inst, 31, 20), 12);
        return QString("%1 x%2, x%3, %4").arg(n[funct3]).arg(rd).arg(rs1).arg(imm);
    }
    case Op::OP: {
        QString n = (funct3 == 0) ? (funct7 == 0x20 ? "sub" : "add")
                  : (funct3 == 1) ? "sll" : (funct3 == 4) ? "xor"
                  : (funct3 == 5) ? "srl" : (funct3 == 6) ? "or"
                  : (funct3 == 7) ? "and" : "op?";
        return QString("%1 x%2, x%3, x%4").arg(n).arg(rd).arg(rs1).arg(rs2);
    }
    case Op::LUI:
        return QString("lui x%1, 0x%2").arg(rd).arg(bits(inst, 31, 12), 0, 16);
    case Op::LOAD: {
        static const char* n[3] = {"lb","lh","lw"};
        i32 imm = signExtend(bits(inst, 31, 20), 12);
        return QString("%1 x%2, %3(x%4)")
                .arg(funct3 < 3 ? n[funct3] : "load?").arg(rd).arg(imm).arg(rs1);
    }
    case Op::STORE: {
        static const char* n[3] = {"sb","sh","sw"};
        i32 imm = signExtend((bits(inst,31,25) << 5) | bits(inst,11,7), 12);
        return QString("%1 x%2, %3(x%4)")
                .arg(funct3 < 3 ? n[funct3] : "store?").arg(rs2).arg(imm).arg(rs1);
    }
    case Op::BRANCH: {
        i32 imm = signExtend((bits(inst,31,31)<<12) | (bits(inst,7,7)<<11)
                           | (bits(inst,30,25)<<5) | (bits(inst,11,8)<<1), 13);
        return QString("%1 x%2, x%3, %4")
                .arg(funct3 == 0 ? "beq" : funct3 == 1 ? "bne" : "b?")
                .arg(rs1).arg(rs2).arg(imm);
    }
    case Op::JAL: {
        i32 imm = signExtend((bits(inst,31,31)<<20) | (bits(inst,30,21)<<1)
                           | (bits(inst,20,20)<<11) | (bits(inst,19,12)<<12), 21);
        return QString("jal x%1, %2").arg(rd).arg(imm);
    }
    default:
        return QString("未知指令 0x%1").arg(inst, 8, 16, QLatin1Char('0'));
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), isRunning(false)
{
    setupUI();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    setWindowTitle("RISC-V 模拟器");
    resize(1100, 750);

    // ===== 创建菜单栏 =====
    QMenuBar *menuBar = new QMenuBar(this);

    // ---- 文件菜单 ----
    QMenu *fileMenu = menuBar->addMenu("文件");
    QAction *actionNew = new QAction("新建", this);
    QAction *actionOpen = new QAction("打开...", this);
    QAction *actionSave = new QAction("保存", this);
    QAction *actionSaveAs = new QAction("另存为...", this);
    QAction *actionExit = new QAction("退出", this);
    fileMenu->addAction(actionNew);
    fileMenu->addAction(actionOpen);
    fileMenu->addAction(actionSave);
    fileMenu->addAction(actionSaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction(actionExit);

    connect(actionNew, &QAction::triggered, this, &MainWindow::onNewFile);
    connect(actionOpen, &QAction::triggered, this, &MainWindow::onOpenClicked);
    connect(actionSave, &QAction::triggered, this, &MainWindow::onSave);
    connect(actionSaveAs, &QAction::triggered, this, &MainWindow::onSaveAs);
    connect(actionExit, &QAction::triggered, this, &MainWindow::onExit);

    // ---- 模拟器菜单 ----
    QMenu *simMenu = menuBar->addMenu("模拟器");
    QAction *actionRun = new QAction("运行全部", this);
    QAction *actionStep = new QAction("单步执行", this);
    QAction *actionPause = new QAction("暂停", this);
    QAction *actionReset = new QAction("重置 CPU", this);
    simMenu->addAction(actionRun);
    simMenu->addAction(actionStep);
    simMenu->addAction(actionPause);
    simMenu->addSeparator();
    simMenu->addAction(actionReset);

    connect(actionRun, &QAction::triggered, this, &MainWindow::onRunClicked);
    connect(actionStep, &QAction::triggered, this, &MainWindow::onStepClicked);
    connect(actionPause, &QAction::triggered, this, &MainWindow::onPause);
    connect(actionReset, &QAction::triggered, this, &MainWindow::onResetClicked);

    // ---- 视图菜单 ----
    QMenu *viewMenu = menuBar->addMenu("视图");
    QAction *actionToggleReg = new QAction("显示寄存器表", this);
    actionToggleReg->setCheckable(true);
    actionToggleReg->setChecked(true);
    QAction *actionToggleMem = new QAction("显示内存表", this);
    actionToggleMem->setCheckable(true);
    actionToggleMem->setChecked(true);
    QAction *actionToggleDatapath = new QAction("显示数据通路", this);
    actionToggleDatapath->setCheckable(true);
    actionToggleDatapath->setChecked(true);
    QAction *actionClearLog = new QAction("清空日志", this);
    QAction *actionRefresh = new QAction("刷新视图", this);
    viewMenu->addAction(actionToggleReg);
    viewMenu->addAction(actionToggleMem);
    viewMenu->addAction(actionToggleDatapath);
    viewMenu->addSeparator();
    viewMenu->addAction(actionClearLog);
    viewMenu->addAction(actionRefresh);

    connect(actionToggleReg, &QAction::triggered, [this](bool checked) {
        onToggleRegTable(checked);
    });
    connect(actionToggleMem, &QAction::triggered, [this](bool checked) {
        onToggleMemTable(checked);
    });
    connect(actionToggleDatapath, &QAction::triggered, [this](bool checked) {
        onToggleDatapath(checked);
    });
    connect(actionClearLog, &QAction::triggered, this, &MainWindow::onClearLog);
    connect(actionRefresh, &QAction::triggered, this, &MainWindow::onRefreshView);

    // ---- 帮助菜单 ----
    QMenu *helpMenu = menuBar->addMenu("帮助");
    QAction *actionHelp = new QAction("已实现指令", this);
    QAction *actionAbout = new QAction("关于", this);
    helpMenu->addAction(actionHelp);
    helpMenu->addSeparator();
    helpMenu->addAction(actionAbout);

    connect(actionHelp, &QAction::triggered, this, &MainWindow::onHelpClicked);
    connect(actionAbout, &QAction::triggered, this, &MainWindow::onAbout);

    // ===== 创建控制按钮 =====
    btnStep = new QPushButton("▶ 单步");
    btnStep->setToolTip("执行一条指令，用于调试");
    btnStep->setFixedWidth(90);

    btnRun = new QPushButton("▶▶ 运行");
    btnRun->setToolTip("执行程序直到结束");
    btnRun->setFixedWidth(90);

    btnPause = new QPushButton("⏸ 暂停");
    btnPause->setToolTip("暂停正在运行的程序");
    btnPause->setFixedWidth(90);
    btnPause->setEnabled(false);

    btnReset = new QPushButton("⟳ 重置");
    btnReset->setToolTip("重置 CPU 状态，清空日志");
    btnReset->setFixedWidth(90);

    btnHelp = new QPushButton("? 帮助");
    btnHelp->setToolTip("查看已实现的指令列表");
    btnHelp->setFixedWidth(90);

    // ===== 运行速度：直接填「每秒几条指令」，比毫秒直观 =====
    QLabel *speedTip = new QLabel("速度：");
    speedBox = new QDoubleSpinBox;
    speedBox->setRange(0.2, 50.0);       // 0.2 条/秒（5 秒一条，超慢）~ 50 条/秒
    speedBox->setDecimals(1);
    speedBox->setSingleStep(0.5);
    speedBox->setValue(2.0);             // 默认 2 条/秒，肉眼舒服
    speedBox->setSuffix(" 条/秒");
    speedBox->setFixedWidth(110);
    speedBox->setToolTip("每秒执行几条指令。填小 = 慢放看灯，填大 = 快速跑完");

    runTimer = new QTimer(this);
    runTimer->setInterval(500);          // 2 条/秒 = 500ms 一拍
    connect(runTimer, &QTimer::timeout, this, &MainWindow::onTimerTick);
    connect(speedBox, &QDoubleSpinBox::valueChanged, this, [this](double ips) {
        runTimer->setInterval(int(1000.0 / ips));   // 条/秒 → 毫秒，内部换算
    });

    // ===== 创建其他控件 =====
    lblPC = new QLabel("PC: 0x00000000");
    lblPC->setStyleSheet("font-weight: bold; font-size: 12px;");

    codeEdit = new QPlainTextEdit;
    codeEdit->setPlaceholderText("在这里输入汇编，例如：\naddi x1, x0, 5\naddi x2, x0, 10\nadd x3, x1, x2");
    codeEdit->setFont(QFont("Consolas", 11));

    regTable = new QTableWidget(32, 2);
    regTable->setHorizontalHeaderLabels({"寄存器", "值"});
    regTable->setFont(QFont("Consolas", 10));
    regTable->setAlternatingRowColors(true);
    // 注释掉暂时解决编译问题
    // regTable->verticalHeader()->setVisible(false);

    memTable = new QTableWidget(0, 2);
    memTable->setHorizontalHeaderLabels({"地址", "数据"});
    memTable->setFont(QFont("Consolas", 10));
    memTable->setAlternatingRowColors(true);
    // 注释掉暂时解决编译问题
    // memTable->verticalHeader()->setVisible(false);

    logEdit = new QTextEdit;
    logEdit->setReadOnly(true);
    logEdit->setFont(QFont("Consolas", 10));
    logEdit->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4;");

    datapath = new DatapathWidget;

    // 用容器包装，方便切换显示
    regContainer = new QWidget;
    QVBoxLayout *regLayout = new QVBoxLayout(regContainer);
    regLayout->setContentsMargins(0, 0, 0, 0);
    regLayout->addWidget(regTable);

    memContainer = new QWidget;
    QVBoxLayout *memLayout = new QVBoxLayout(memContainer);
    memLayout->setContentsMargins(0, 0, 0, 0);
    memLayout->addWidget(memTable);

    datapathContainer = new QWidget;
    QVBoxLayout *dpLayout = new QVBoxLayout(datapathContainer);
    dpLayout->setContentsMargins(0, 0, 0, 0);
    dpLayout->addWidget(datapath);

    // 初始化寄存器表
    for (int i = 0; i < 32; ++i) {
        regTable->setItem(i, 0, new QTableWidgetItem(QString("x%1").arg(i)));
        regTable->setItem(i, 1, new QTableWidgetItem("0x00000000"));
    }

    // ===== 布局 =====
    QHBoxLayout *topBar = new QHBoxLayout;
    topBar->addWidget(menuBar);
    topBar->addStretch();
    topBar->addWidget(btnStep);
    topBar->addWidget(btnRun);
    topBar->addWidget(btnPause);
    topBar->addWidget(btnReset);
    topBar->addWidget(btnHelp);
    topBar->addWidget(speedTip);
    topBar->addWidget(speedBox);
    topBar->addStretch();
    topBar->addWidget(lblPC);

    QHBoxLayout *middle = new QHBoxLayout;
    middle->addWidget(codeEdit, 1);
    middle->addWidget(regContainer, 1);

    QHBoxLayout *bottom = new QHBoxLayout;
    bottom->addWidget(memContainer, 1);
    bottom->addWidget(logEdit, 1);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(topBar);
    mainLayout->addLayout(middle, 1);
    mainLayout->addLayout(bottom, 1);
    mainLayout->addWidget(datapathContainer);

    QWidget *central = new QWidget(this);
    central->setLayout(mainLayout);
    setCentralWidget(central);

    // ===== 状态栏 =====
    statusBar()->showMessage("就绪");

    // ===== 连接按钮信号槽 =====
    connect(btnStep, &QPushButton::clicked, this, &MainWindow::onStepClicked);
    connect(btnRun, &QPushButton::clicked, this, &MainWindow::onRunClicked);
    connect(btnPause, &QPushButton::clicked, this, &MainWindow::onPause);
    connect(btnReset, &QPushButton::clicked, this, &MainWindow::onResetClicked);
    connect(btnHelp, &QPushButton::clicked, this, &MainWindow::onHelpClicked);
}

void MainWindow::refreshUI()
{
    for (int i = 0; i < 32; ++i) {
        regTable->item(i, 1)->setText(QString("0x%1").arg(cpu.reg(i), 8, 16, QLatin1Char('0')));
    }
    lblPC->setText(QString("PC: 0x%1").arg(cpu.pc(), 8, 16, QLatin1Char('0')));

    // 复用已有单元格，不再每拍 new 一堆 QTableWidgetItem（否则界面卡，灯会被跳过）
    for (int i = 0; i < memTable->rowCount(); ++i) {
        QString v = QString("0x%1").arg(cpu.mem().loadWord(i * 4), 8, 16, QLatin1Char('0'));
        if (QTableWidgetItem *it = memTable->item(i, 1)) it->setText(v);
        else memTable->setItem(i, 1, new QTableWidgetItem(v));
    }

    updateDatapath();
    // 关键：update() 只是「排队重绘」，连续几拍会被 Qt 合并成一次，
    // 结果就是只看到最后一条指令的灯。repaint() 立刻画完，每一拍都看得见。
    datapath->repaint();
    statusBar()->showMessage(QString("PC: 0x%1").arg(cpu.pc(), 8, 16, QLatin1Char('0')));
}

uint32_t MainWindow::datapathMask() const
{
    // 按「最近执行的那条指令」的 opcode 决定亮哪些部件
    u32 op = bits(cpu.inst(), 6, 0);
    uint32_t m = DatapathWidget::STAGE_PC | DatapathWidget::STAGE_IF | DatapathWidget::STAGE_ID;

    switch (op) {
    case Op::OP_IMM:
    case Op::OP:      // 运算类：寄存器堆 + ALU + 写回
        m |= DatapathWidget::STAGE_REG | DatapathWidget::STAGE_EX | DatapathWidget::STAGE_WB;
        break;
    case Op::LUI:     // 高位立即数：直接写回
        m |= DatapathWidget::STAGE_WB;
        break;
    case Op::LOAD:    // 读内存：访存 + 写回
        m |= DatapathWidget::STAGE_REG | DatapathWidget::STAGE_EX |
             DatapathWidget::STAGE_MEM | DatapathWidget::STAGE_WB;
        break;
    case Op::STORE:   // 写内存：有访存、无写回
        m |= DatapathWidget::STAGE_REG | DatapathWidget::STAGE_EX | DatapathWidget::STAGE_MEM;
        break;
    case Op::BRANCH:  // 分支：只用寄存器堆 + ALU，且可能改写 PC
        m |= DatapathWidget::STAGE_REG | DatapathWidget::STAGE_EX | DatapathWidget::STAGE_PCWR;
        break;
    case Op::JAL:     // 跳转：写返回地址 + 改写 PC
        m |= DatapathWidget::STAGE_WB | DatapathWidget::STAGE_PCWR;
        break;
    default:
        return 0;     // 不认识就不亮
    }
    return m;
}

void MainWindow::updateDatapath()
{
    datapath->setHighlightMask(datapathMask());
    // 图上直接写清「这是第几拍、走的哪条指令」，防止怀疑灯没在变
    if (cpu.inst() == 0)
        datapath->setInstText("尚未执行指令");
    else
        datapath->setInstText(QString("第 %1 拍：%2   （PC → 0x%3）")
                                  .arg(tickCount)
                                  .arg(describeInst(cpu.inst()))
                                  .arg(cpu.pc(), 8, 16, QLatin1Char('0')));
}

void MainWindow::loadProgram()
{
    try {
        auto code = assembler.assemble(codeEdit->toPlainText().toStdString());
        cpu.reset();
        cpu.loadProgram(code);
        programLoaded = true;
        memTable->setRowCount((int)code.size());
        for (int i = 0; i < (int)code.size(); ++i) {
            memTable->setItem(i, 0, new QTableWidgetItem(QString("0x%1").arg(i * 4, 2, 16, QLatin1Char('0'))));
        }
        logEdit->append(QString("✓ 已加载 %1 条指令").arg(code.size()));
        refreshUI();
        statusBar()->showMessage(QString("已加载 %1 条指令").arg(code.size()), 3000);
    } catch (const std::exception& e) {
        logEdit->append(QString("✗ 汇编错误：%1").arg(e.what()));
        statusBar()->showMessage("汇编错误", 3000);
    }
}

// ===== 文件菜单 =====

void MainWindow::onNewFile()
{
    if (!codeEdit->toPlainText().isEmpty()) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "新建文件",
                                      "当前内容未保存，是否继续？",
                                      QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No) return;
    }
    codeEdit->clear();
    currentFilePath.clear();
    logEdit->append("新建文件");
    statusBar()->showMessage("已新建文件", 2000);
}

void MainWindow::onOpenClicked()
{
    QString path = QFileDialog::getOpenFileName(this, "打开汇编文件", "",
                                                "文本文件 (*.txt *.asm);;所有文件 (*)");
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        logEdit->append("✗ 无法打开文件：" + path);
        return;
    }
    QTextStream in(&file);
    codeEdit->setPlainText(in.readAll());
    file.close();
    currentFilePath = path;
    logEdit->append("✓ 已打开：" + path);
    statusBar()->showMessage("已打开：" + path, 3000);
}

void MainWindow::onSave()
{
    if (currentFilePath.isEmpty()) {
        onSaveAs();
        return;
    }

    QFile file(currentFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        logEdit->append("✗ 无法保存文件：" + currentFilePath);
        return;
    }
    QTextStream out(&file);
    out << codeEdit->toPlainText();
    file.close();
    logEdit->append("✓ 已保存：" + currentFilePath);
    statusBar()->showMessage("已保存：" + currentFilePath, 3000);
}

void MainWindow::onSaveAs()
{
    QString path = QFileDialog::getSaveFileName(this, "保存汇编文件", "",
                                                "文本文件 (*.txt *.asm);;所有文件 (*)");
    if (path.isEmpty()) return;

    if (!path.endsWith(".txt") && !path.endsWith(".asm")) {
        path += ".asm";
    }

    currentFilePath = path;
    onSave();
}

void MainWindow::onExit()
{
    close();
}

//模拟器菜单

void MainWindow::onStepClicked()
{
    try {
        if (!programLoaded) loadProgram();
        if (cpu.finished()) {
            logEdit->append("程序已执行完，点「重置」重新开始");
            statusBar()->showMessage("程序已执行完", 2000);
            return;
        }
        cpu.step();
        ++tickCount;
        refreshUI();
        logEdit->append(QString("[%1] %2").arg(tickCount, 3).arg(describeInst(cpu.inst())));
        statusBar()->showMessage("单步执行完成", 2000);
    } catch (const std::exception& e) {
        logEdit->append(QString("✗ 运行时错误：%1").arg(e.what()));
        statusBar()->showMessage("运行时错误", 3000);
    }
}

void MainWindow::onRunClicked()
{
    if (isRunning) return;

    loadProgram();               // 重新汇编 + 装载（loadProgram 内部已 try/catch）
    if (cpu.finished()) {        // 什么都没装进去
        logEdit->append("没有可执行的指令");
        statusBar()->showMessage("没有可执行的指令", 2000);
        return;
    }

    isRunning = true;
    tickCount = 0;
    btnRun->setEnabled(false);
    btnPause->setEnabled(true);
    btnStep->setEnabled(false);
    statusBar()->showMessage("正在运行...（灯随指令走）");
    runTimer->setInterval(int(1000.0 / speedBox->value()));  // 按输入框当前值定拍
    runTimer->start();           // QTimer 驱动：每拍走一条，界面跟着刷新
}

void MainWindow::onTimerTick()
{
    try {
        if (cpu.finished()) {        // 跑完了，收工
            runTimer->stop();
            isRunning = false;
            btnRun->setEnabled(true);
            btnPause->setEnabled(false);
            btnStep->setEnabled(true);
            refreshUI();
            logEdit->append(QString("✓ 程序执行完成，共 %1 条").arg(tickCount));
            statusBar()->showMessage("程序执行完成", 3000);
            return;
        }
        cpu.step();                  // 一次只走一条
        ++tickCount;
        refreshUI();                 // 寄存器/内存/数据通路灯一起刷
        // 每拍都写日志：如果日志里有 N 行，就说明灯确实走了 N 次
        logEdit->append(QString("[%1] %2").arg(tickCount, 3).arg(describeInst(cpu.inst())));
    } catch (const std::exception& e) {
        runTimer->stop();
        isRunning = false;
        btnRun->setEnabled(true);
        btnPause->setEnabled(false);
        btnStep->setEnabled(true);
        logEdit->append(QString("✗ 运行时错误：%1").arg(e.what()));
        statusBar()->showMessage("运行时错误", 3000);
    }
}

void MainWindow::onPause()
{
    if (!isRunning) return;
    runTimer->stop();
    isRunning = false;
    btnRun->setEnabled(true);
    btnPause->setEnabled(false);
    btnStep->setEnabled(true);
    logEdit->append("⏸ 已暂停");
    statusBar()->showMessage("已暂停", 2000);
}

void MainWindow::onResetClicked()
{
    runTimer->stop();
    isRunning = false;
    tickCount = 0;
    btnRun->setEnabled(true);
    btnPause->setEnabled(false);
    btnStep->setEnabled(true);

    cpu.reset();
    programLoaded = false;
    refreshUI();
    logEdit->clear();
    logEdit->append("已重置 CPU");
    statusBar()->showMessage("已重置", 2000);
}

//视图菜单

void MainWindow::onToggleRegTable(bool checked)
{
    regContainer->setVisible(checked);
}

void MainWindow::onToggleMemTable(bool checked)
{
    memContainer->setVisible(checked);
}

void MainWindow::onToggleDatapath(bool checked)
{
    datapathContainer->setVisible(checked);
}

void MainWindow::onClearLog()
{
    logEdit->clear();
    logEdit->append("日志已清空");
    statusBar()->showMessage("日志已清空", 2000);
}

void MainWindow::onRefreshView()
{
    refreshUI();
    statusBar()->showMessage("视图已刷新", 2000);
}

//帮助菜单

void MainWindow::onHelpClicked()
{
    QMessageBox::information(this, "已实现的指令",
                             "当前已实现 19 条指令（RV32I 子集）：\n\n"
                             "【算术】add、sub、addi\n"
                             "【逻辑】andi、ori、xori\n"
                             "【移位】slli\n"
                             "【比较】slti、sltiu\n"
                             "【高位立即数】lui\n"
                             "【分支】beq、bne\n"
                             "【跳转】jal\n"
                             "【访存·读】lw、lb、lh\n"
                             "【访存·写】sw、sb、sh\n\n"
                             "还没实现：jalr、auipc、sll/srl/sra、slt/sltu、lbu/lhu、blt/bge/bltu/bgeu 等");
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "关于 RISC-V 模拟器",
                       "<h2>RISC-V 模拟器</h2>"
                       "<p>版本：1.0.0</p>"
                       "<p>基于 Qt 6 开发的 RISC-V 32I 指令集模拟器</p>"
                       "<p>支持 RV32I 基础指令集（19 条指令）</p>"
                       "<p>项目地址：<a href='https://github.com'>GitHub</a></p>"
                       "<p><i>用于计算机组成原理课程学习</i></p>");
}
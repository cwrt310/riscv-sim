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
    setMinimumHeight(200);
}

void DatapathWidget::setHighlightStage(Stage stage)
{
    m_highlightStage = stage;
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

    int boxW = 110;
    int boxH = 50;
    int y = 50;
    int spacing = 30;

    int xPC = 20;
    int xIF = xPC + boxW + spacing;
    int xID = xIF + boxW + spacing;
    int xREG = xID + boxW + spacing;
    int xEX = xREG + boxW + spacing;
    int xMEM = xEX + boxW + spacing;
    int xWB = xMEM + boxW + spacing;

    int midY = y + boxH / 2;

    auto isHighlight = [&](Stage s) {
        return m_highlightStage == s;
    };

    auto drawBox = [&](int x, int y, int w, int h, const QString& label,
                       bool highlight) {
        if (highlight) {
            p.setBrush(highlightColor);
            p.setPen(QPen(QColor(200, 120, 30), 3));
        } else {
            p.setBrush(normalColor);
            p.setPen(QPen(borderColor, 2));
        }
        p.drawRect(x, y, w, h);
        p.setPen(QPen(textColor, 1));
        p.drawText(x + (w - QFontMetrics(p.font()).horizontalAdvance(label)) / 2,
                   y + h / 2 + 6, label);
    };

    auto drawArrowH = [&](int x1, int x2) {
        int dir = (x2 > x1) ? 1 : -1;
        int tipX = x2 - dir * 8;
        p.setPen(QPen(lineColor, 2));
        p.drawLine(x1, midY, tipX, midY);
        QPolygon arrow;
        arrow << QPoint(x2, midY)
              << QPoint(x2 - dir * 10, midY - 6)
              << QPoint(x2 - dir * 10, midY + 6);
        p.setBrush(lineColor);
        p.drawPolygon(arrow);
    };

    drawBox(xPC, y, boxW, boxH, "PC", isHighlight(STAGE_PC));
    drawBox(xIF, y, boxW, boxH, "指令存储器", isHighlight(STAGE_IF));
    drawBox(xID, y, boxW, boxH, "译码", isHighlight(STAGE_ID));
    drawBox(xREG, y, boxW, boxH, "寄存器堆", isHighlight(STAGE_REG));
    drawBox(xEX, y, boxW, boxH, "ALU", isHighlight(STAGE_EX));
    drawBox(xMEM, y, boxW, boxH, "数据存储器", isHighlight(STAGE_MEM));
    drawBox(xWB, y, boxW, boxH, "写回", isHighlight(STAGE_WB));

    drawArrowH(xPC + boxW, xIF);
    drawArrowH(xIF + boxW, xID);
    drawArrowH(xID + boxW, xREG);
    drawArrowH(xREG + boxW, xEX);
    drawArrowH(xEX + boxW, xMEM);

    p.setPen(QPen(lineColor, 2));
    int memCenterX = xMEM + boxW / 2;
    int wbCenterX = xWB + boxW / 2;
    int turnY = y + boxH + 30;
    p.drawLine(memCenterX, y + boxH, memCenterX, turnY);
    p.drawLine(memCenterX, turnY, wbCenterX, turnY);
    p.drawLine(wbCenterX, turnY, wbCenterX, y + boxH);

    QPolygon arrowV;
    arrowV << QPoint(wbCenterX, y + boxH)
           << QPoint(wbCenterX - 6, y + boxH - 10)
           << QPoint(wbCenterX + 6, y + boxH - 10);
    p.setBrush(lineColor);
    p.drawPolygon(arrowV);

    p.setPen(QPen(QColor(100, 100, 100), 1));
    p.drawText(xIF + 38, y + boxH + 20, "IF");
    p.drawText(xID + 38, y + boxH + 20, "ID");
    p.drawText(xREG + 32, y + boxH + 20, "REG");
    p.drawText(xEX + 38, y + boxH + 20, "EX");
    p.drawText(xMEM + 32, y + boxH + 20, "MEM");
    p.drawText(xWB + 38, y + boxH + 20, "WB");
}

//MainWindow实现

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

    for (int i = 0; i < memTable->rowCount(); ++i) {
        memTable->setItem(i, 1, new QTableWidgetItem(QString("0x%1").arg(cpu.mem().loadWord(i * 4), 8, 16, QLatin1Char('0'))));
    }

    updateDatapath();
    statusBar()->showMessage(QString("PC: 0x%1").arg(cpu.pc(), 8, 16, QLatin1Char('0')));
}

void MainWindow::updateDatapath()
{
    DatapathWidget::Stage stage = DatapathWidget::STAGE_NONE;

    if (cpu.finished()) {
        stage = DatapathWidget::STAGE_NONE;
    } else {
        uint32_t pc = cpu.pc();
        int idx = (pc / 4) % 7;
        switch(idx) {
        case 0: stage = DatapathWidget::STAGE_PC; break;
        case 1: stage = DatapathWidget::STAGE_IF; break;
        case 2: stage = DatapathWidget::STAGE_ID; break;
        case 3: stage = DatapathWidget::STAGE_REG; break;
        case 4: stage = DatapathWidget::STAGE_EX; break;
        case 5: stage = DatapathWidget::STAGE_MEM; break;
        case 6: stage = DatapathWidget::STAGE_WB; break;
        }
    }

    datapath->setHighlightStage(stage);
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
        refreshUI();
        statusBar()->showMessage("单步执行完成", 2000);
    } catch (const std::exception& e) {
        logEdit->append(QString("✗ 运行时错误：%1").arg(e.what()));
        statusBar()->showMessage("运行时错误", 3000);
    }
}

void MainWindow::onRunClicked()
{
    if (isRunning) return;

    try {
        loadProgram();
        isRunning = true;
        btnRun->setEnabled(false);
        btnPause->setEnabled(true);
        btnStep->setEnabled(false);
        statusBar()->showMessage("正在运行...");

        cpu.run();

        isRunning = false;
        btnRun->setEnabled(true);
        btnPause->setEnabled(false);
        btnStep->setEnabled(true);
        refreshUI();
        logEdit->append("✓ 程序执行完成");
        statusBar()->showMessage("程序执行完成", 3000);
    } catch (const std::exception& e) {
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
    // TODO: 需要修改 CPU 后端支持暂停
    // 目前 CPU::run() 是同步循环，无法从外部中断
    logEdit->append("暂停功能需要 CPU 后端支持（待实现）");
    statusBar()->showMessage("暂停功能待实现", 2000);
}

void MainWindow::onResetClicked()
{
    isRunning = false;
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
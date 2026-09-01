#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include "CPU.h"
#include "Assembler.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    CPU cpu;
    Assembler as;
    bool loaded = false;          // 程序是否已加载（单步要靠它判断）

    // ===== 窗口 + 控件（第②步骨架）=====
    QWidget window;
    window.setWindowTitle("RISC-V 模拟器");
    window.resize(1000, 700);

    QPushButton* btnOpen  = new QPushButton("打开");
    QPushButton* btnStep  = new QPushButton("单步");
    QPushButton* btnRun   = new QPushButton("运行");
    QPushButton* btnReset = new QPushButton("重置");
    QPushButton* btnHelp  = new QPushButton("帮助");
    QLabel* lblPC = new QLabel("PC: 0x00000000");

    QHBoxLayout* topBar = new QHBoxLayout;
    topBar->addWidget(btnOpen);
    topBar->addWidget(btnStep);
    topBar->addWidget(btnRun);
    topBar->addWidget(btnReset);
    topBar->addWidget(btnHelp);
    topBar->addStretch();
    topBar->addWidget(lblPC);

    QPlainTextEdit* codeEdit = new QPlainTextEdit;
    codeEdit->setPlaceholderText("在这里输入汇编，例如：\naddi x1, x0, 5");

    QTableWidget* regTable = new QTableWidget(32, 2);
    regTable->setHorizontalHeaderLabels({"寄存器", "值"});

    QTableWidget* memTable = new QTableWidget(0, 2);   // 初始 0 行，加载后按程序大小设置
    memTable->setHorizontalHeaderLabels({"地址", "数据"});

    QTextEdit* logEdit = new QTextEdit;
    logEdit->setReadOnly(true);

    QHBoxLayout* middle = new QHBoxLayout;
    middle->addWidget(codeEdit, 1);
    middle->addWidget(regTable, 1);

    QHBoxLayout* bottom = new QHBoxLayout;
    bottom->addWidget(memTable, 1);
    bottom->addWidget(logEdit, 1);

    QVBoxLayout* mainLayout = new QVBoxLayout(&window);
    mainLayout->addLayout(topBar);
    mainLayout->addLayout(middle, 1);
    mainLayout->addLayout(bottom, 1);

    // ===== 初始化寄存器表（内存表等加载后再填）=====
    for (int i = 0; i < 32; ++i) {
        regTable->setItem(i, 0, new QTableWidgetItem(QString("x%1").arg(i)));
        regTable->setItem(i, 1, new QTableWidgetItem("0x00000000"));
    }

    // ===== 刷新：把 CPU 状态填进界面 =====
    auto refresh = [&]() {
        for (int i = 0; i < 32; ++i)
            regTable->item(i, 1)->setText(QString("0x%1").arg(cpu.reg(i), 8, 16, QLatin1Char('0')));
        lblPC->setText(QString("PC: 0x%1").arg(cpu.pc(), 8, 16, QLatin1Char('0')));
        for (int i = 0; i < memTable->rowCount(); ++i)
            memTable->setItem(i, 1, new QTableWidgetItem(QString("0x%1").arg(cpu.mem().loadWord(i * 4), 8, 16, QLatin1Char('0'))));
    };

    // ===== 加载：汇编 + 装载（不执行）=====
    auto load = [&]() {
        auto code = as.assemble(codeEdit->toPlainText().toStdString());
        cpu.reset();
        cpu.loadProgram(code);
        loaded = true;
        memTable->setRowCount(int(code.size()));           // 内存表行数 = 指令条数
        for (int i = 0; i < int(code.size()); ++i)         // 填地址列
            memTable->setItem(i, 0, new QTableWidgetItem(
                QString("0x%1").arg(i * 4, 2, 16, QLatin1Char('0'))));
        logEdit->append(QString("已加载 %1 条指令").arg(code.size()));
        refresh();
    };

    // ===== 按钮的逻辑 =====
    QObject::connect(btnOpen, &QPushButton::clicked, [&]() {
        QString path = QFileDialog::getOpenFileName(&window, "打开汇编文件", "", "文本文件 (*.txt *.asm);;所有文件 (*)");
        if (path.isEmpty()) return;                    // 用户取消了
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            logEdit->append("无法打开文件：" + path);
            return;
        }
        QTextStream in(&file);
        codeEdit->setPlainText(in.readAll());           // 文件内容填进输入框
        file.close();
        logEdit->append("已打开：" + path);
    });

    QObject::connect(btnRun, &QPushButton::clicked, [&]() {
        load();
        cpu.run();
        refresh();
        logEdit->append("程序执行完成");
    });

    QObject::connect(btnStep, &QPushButton::clicked, [&]() {
        if (!loaded) load();      // 还没加载就先加载
        if (cpu.finished()) {     // 程序已执行完，再单步会越界读垃圾
            logEdit->append("程序已执行完，点「重置」重新开始");
            return;
        }
        cpu.step();               // 只走一条
        refresh();
    });

    QObject::connect(btnReset, &QPushButton::clicked, [&]() {
        cpu.reset();
        loaded = false;
        refresh();
        logEdit->clear();
    });

   QObject::connect(btnHelp, &QPushButton::clicked, [&]() {
    QMessageBox::information(&window, "已实现的指令",
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
    });

    window.show();
    return app.exec();
}
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QTableWidget>
#include <QPushButton>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QCheckBox>
#include <QStatusBar>
#include <QDoubleSpinBox>
#include <QTimer>
#include "CPU.h"
#include "Assembler.h"
#include <QHeaderView>

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

class DatapathWidget : public QWidget {
    Q_OBJECT
public:
    // 位掩码：每一位代表一个部件，一次可同时亮多个
    enum StageFlag {
        STAGE_NONE = 0,
        STAGE_PC   = 1 << 0,
        STAGE_IF   = 1 << 1,
        STAGE_ID   = 1 << 2,
        STAGE_REG  = 1 << 3,
        STAGE_EX   = 1 << 4,
        STAGE_MEM  = 1 << 5,
        STAGE_WB   = 1 << 6,
        STAGE_PCWR = 1 << 7,   // PC 被改写（分支/跳转）
    };

    explicit DatapathWidget(QWidget *parent = nullptr);
    void setHighlightMask(uint32_t mask);
    void setInstText(const QString& text);   // 图顶部显示「第几拍 + 当前指令」

protected:
    void paintEvent(QPaintEvent*) override;

private:
    uint32_t m_mask = STAGE_NONE;
    QString  m_instText;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 文件菜单
    void onNewFile();
    void onOpenClicked();
    void onSave();
    void onSaveAs();
    void onExit();

    // 模拟器菜单
    void onRunClicked();
    void onStepClicked();
    void onResetClicked();
    void onPause();

    // 视图菜单
    void onToggleRegTable(bool checked);
    void onToggleMemTable(bool checked);
    void onToggleDatapath(bool checked);
    void onClearLog();
    void onRefreshView();

    // 帮助菜单
    void onHelpClicked();
    void onAbout();

    void refreshUI();
    void onTimerTick();             // 运行定时器：每拍执行一条
    uint32_t datapathMask() const;  // 按当前指令算该亮哪些部件

private:
    // 后端
    CPU cpu;
    Assembler assembler;
    bool programLoaded = false;
    bool isRunning = false;        // 是否正在运行
    QString currentFilePath;

    // 界面控件
    // 控制按钮
    QPushButton *btnStep;
    QPushButton *btnRun;
    QPushButton *btnPause;
    QPushButton *btnReset;
    QPushButton *btnHelp;
    QLabel *lblPC;
    QPlainTextEdit *codeEdit;
    QTableWidget *regTable;
    QTableWidget *memTable;
    QTextEdit *logEdit;
    DatapathWidget *datapath;

    // 运行速度控制
    QDoubleSpinBox *speedBox;      // 每秒执行几条指令（直观，不用算毫秒）
    QTimer *runTimer;
    int  tickCount = 0;            // 已执行拍数，给数据通路图上的提示用

    // 视图切换用
    QWidget *regContainer;
    QWidget *memContainer;
    QWidget *datapathContainer;

    void setupUI();
    void loadProgram();
    void updateDatapath();
};

#endif // MAINWINDOW_H
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
#include "CPU.h"
#include "Assembler.h"
#include <QHeaderView>

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

class DatapathWidget : public QWidget {
    Q_OBJECT
public:
    enum Stage {
        STAGE_PC,
        STAGE_IF,
        STAGE_ID,
        STAGE_REG,
        STAGE_EX,
        STAGE_MEM,
        STAGE_WB,
        STAGE_NONE
    };

    explicit DatapathWidget(QWidget *parent = nullptr);
    void setHighlightStage(Stage stage);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    Stage m_highlightStage = STAGE_NONE;
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

    // 视图切换用
    QWidget *regContainer;
    QWidget *memContainer;
    QWidget *datapathContainer;

    void setupUI();
    void loadProgram();
    void updateDatapath();
};

#endif // MAINWINDOW_H
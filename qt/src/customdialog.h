// customdialog.h — 自定义难度对话框（QDialog，由 Win32 版 dialogs.cpp 移植）
#pragma once

#include <QDialog>

class QLineEdit;

class CustomDialog : public QDialog {
    Q_OBJECT
public:
    explicit CustomDialog(int rows, int cols, int mines, QWidget* parent = nullptr);

    // 显示"自定义难度"对话框；返回 true 表示确定，并回填新的行列雷数
    static bool getDifficulty(QWidget* parent, int& rows, int& cols, int& mines);

protected:
    void accept() override;   // 确定时校验输入（范围与原版一致）

private:
    QLineEdit* m_rowsEdit  = nullptr;
    QLineEdit* m_colsEdit  = nullptr;
    QLineEdit* m_minesEdit = nullptr;

    int m_resultRows  = 0;
    int m_resultCols  = 0;
    int m_resultMines = 0;
};

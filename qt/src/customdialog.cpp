// customdialog.cpp — 自定义难度对话框实现（由 Win32 版 dialogs.cpp 移植）
#include "customdialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QIntValidator>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

CustomDialog::CustomDialog(int rows, int cols, int mines, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(QStringLiteral("自定义难度"));

    m_rowsEdit  = new QLineEdit(QString::number(rows), this);
    m_colsEdit  = new QLineEdit(QString::number(cols), this);
    m_minesEdit = new QLineEdit(QString::number(mines), this);
    // 仅允许输入数字（对应原版 ES_NUMBER）
    auto* validator = new QIntValidator(0, 999999, this);
    m_rowsEdit->setValidator(validator);
    m_colsEdit->setValidator(validator);
    m_minesEdit->setValidator(validator);

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("行数:"), m_rowsEdit);
    form->addRow(QStringLiteral("列数:"), m_colsEdit);
    form->addRow(QStringLiteral("地雷数:"), m_minesEdit);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);

    m_rowsEdit->setFocus();
}

void CustomDialog::accept() {
    bool okR = false, okC = false, okM = false;
    int r  = m_rowsEdit->text().toInt(&okR);
    int c  = m_colsEdit->text().toInt(&okC);
    int mn = m_minesEdit->text().toInt(&okM);

    if (!okR || !okC || !okM || r < 5 || r > 40 || c < 5 || c > 60) {
        QMessageBox::warning(this, QStringLiteral("输入有误"),
                             QStringLiteral("行数范围 5–40，列数范围 5–60。"));
        return;
    }
    int maxm = r * c - 1;
    if (mn < 1 || mn > maxm) {
        QMessageBox::warning(this, QStringLiteral("输入有误"),
                             QStringLiteral("地雷数量需在 1–%1 之间。").arg(maxm));
        return;
    }
    m_resultRows  = r;
    m_resultCols  = c;
    m_resultMines = mn;
    QDialog::accept();
}

bool CustomDialog::getDifficulty(QWidget* parent, int& rows, int& cols, int& mines) {
    CustomDialog dlg(rows, cols, mines, parent);
    if (dlg.exec() == QDialog::Accepted) {
        rows  = dlg.m_resultRows;
        cols  = dlg.m_resultCols;
        mines = dlg.m_resultMines;
        return true;
    }
    return false;
}

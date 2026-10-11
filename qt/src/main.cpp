// main.cpp — 程序入口（QApplication，含演示参数）
#include <QApplication>
#include <QTimer>

#include <cstdlib>
#include <ctime>

#include "mainwindow.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("扫雷"));

    std::srand((unsigned)std::time(nullptr));

    MainWindow w;
    w.show();

    // 演示参数（用于截图验证与展示）：
    //   --demo-lose       300ms 后自动摆出"错旗 + 踩雷"失败局面
    //   --demo-win        300ms 后自动翻完安全格触发胜利
    //   --demo-custom     300ms 后打开自定义难度对话框
    //   --demo-cells X    300ms 后注入测试整板 X ∈ {lost, won789, won046}（对齐 GDI 地面真相）
    const QStringList args = QCoreApplication::arguments();
    if (args.contains(QStringLiteral("--demo-lose"))) {
        QTimer::singleShot(300, &w, &MainWindow::demoLoseSetup);
    } else if (args.contains(QStringLiteral("--demo-win"))) {
        QTimer::singleShot(300, &w, &MainWindow::demoWinSetup);
    } else if (args.contains(QStringLiteral("--demo-custom"))) {
        QTimer::singleShot(300, &w, [&w] { w.demoSetupCustom(16, 16, 40); });
    } else {
        int idx = args.indexOf(QStringLiteral("--demo-cells"));
        if (idx >= 0 && idx + 1 < args.size()) {
            const QString kind = args.at(idx + 1);
            QTimer::singleShot(300, &w, [&w, kind] { w.demoCells(kind); });
        }
    }

    return app.exec();
}

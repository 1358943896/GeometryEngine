#include "app/MainWindow.h"
#include <QApplication>
#include <QSurfaceFormat>

int main(int argc, char *argv[])
{
    QSurfaceFormat fmt;
    fmt.setVersion(3, 2);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setStencilBufferSize(8);  // 启用 stencil buffer 用于岛洞多边形绘制
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication app(argc, argv);

    MainWindow mainWin;
    mainWin.show();

    return QApplication::exec();
}

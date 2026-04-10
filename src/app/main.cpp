#include "GeometryWidget.h"
#include "app/Layer.h"
#include "app/LayerManagerWidget.h"
#include "dataengine/WktFileReader.h"
#include <QApplication>
#include <QSurfaceFormat>
#include <QMainWindow>
#include <QMenuBar>
#include <QSplitter>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>

int main(int argc, char *argv[])
{
    QSurfaceFormat fmt;
    fmt.setVersion(3, 2);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setStencilBufferSize(8);  // 启用 stencil buffer 用于岛洞多边形绘制
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication app(argc, argv);

    QMainWindow mainWin;
    mainWin.setWindowTitle("GeometryViewer");
    mainWin.resize(1100, 740);

    // 创建中央分割器
    auto *splitter = new QSplitter(Qt::Horizontal, &mainWin);

    // 创建 GL 控件
    auto *glWidget = new GeometryWidget;

    // 创建图层管理控件
    auto *layerManager = new LayerManagerWidget;

    splitter->addWidget(glWidget);
    splitter->addWidget(layerManager);
    splitter->setSizes({850, 250});
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);

    mainWin.setCentralWidget(splitter);

    // 存储图层对象（保持生命周期）
    std::vector<std::unique_ptr<Layer>> layerStore;

    // 更新 GL 控件的函数
    auto updateGLWidget = [&]() {
        std::vector<Layer *> layerPtrs;
        layerPtrs.reserve(layerStore.size());
        for (const auto &layer : layerStore)
            layerPtrs.push_back(layer.get());
        glWidget->setLayers(std::move(layerPtrs));
    };

    // 菜单：文件
    auto *fileMenu = mainWin.menuBar()->addMenu("文件(&F)");

    // 打开文件
    auto *actOpen = fileMenu->addAction("打开(&O)...");
    actOpen->setShortcut(QKeySequence::Open);

    QObject::connect(actOpen, &QAction::triggered, &mainWin,
        [&]() {
            QString path = QFileDialog::getOpenFileName(
                &mainWin, "打开 WKT 文件", {}, "文本文件 (*.txt *.wkt);;所有文件 (*)");
            if (path.isEmpty()) return;
            try {
                dataengine::WktFileReader reader(path.toStdString());
                auto geometries = reader.read();

                // 从路径提取文件名作为图层名
                QString layerName = QFileInfo(path).fileName();

                // 创建图层
                auto layer = std::make_unique<Layer>(layerName, std::move(geometries));
                layerManager->addLayer(layer.get());
                layerStore.push_back(std::move(layer));

                updateGLWidget();
            } catch (const std::exception &e) {
                QMessageBox::critical(&mainWin, "读取失败", e.what());
            }
        });

    // 清除所有图层
    auto *actClear = fileMenu->addAction("清除所有(&C)");
    QObject::connect(actClear, &QAction::triggered, &mainWin,
        [&]() {
            layerStore.clear();
            layerManager->clearLayers();
            updateGLWidget();
        });

    // 退出
    auto *actExit = fileMenu->addAction("退出(&X)");
    actExit->setShortcut(QKeySequence::Quit);
    QObject::connect(actExit, &QAction::triggered, &mainWin, &QMainWindow::close);

    // 连接图层变化信号
    QObject::connect(layerManager, &LayerManagerWidget::layerChanged,
                     [&]() {
                         glWidget->update();
                     });

    mainWin.show();
    return QApplication::exec();
}
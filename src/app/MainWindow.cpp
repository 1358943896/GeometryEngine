#include "app/MainWindow.h"
#include "GeometryWidget.h"
#include "app/LayerManagerWidget.h"
#include "dataengine/WktFileReader.h"

#include <QSplitter>
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("GeometryViewer");
    resize(1100, 740);

    // 创建中央分割器
    auto *splitter = new QSplitter(Qt::Horizontal, this);

    // 创建 GL 控件
    glWidget_ = new GeometryWidget;

    // 创建图层管理控件
    layerManager_ = new LayerManagerWidget;

    splitter->addWidget(glWidget_);
    splitter->addWidget(layerManager_);
    splitter->setSizes({850, 250});
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);

    setCentralWidget(splitter);

    // 菜单：文件
    auto *fileMenu = menuBar()->addMenu("文件(&F)");

    // 打开文件
    auto *actOpen = fileMenu->addAction("打开(&O)...");
    actOpen->setShortcut(QKeySequence::Open);
    connect(actOpen, &QAction::triggered, this, &MainWindow::onOpenFile);

    // 清除所有图层
    auto *actClear = fileMenu->addAction("清除所有(&C)");
    connect(actClear, &QAction::triggered, this, &MainWindow::onClearAll);

    // 退出
    auto *actExit = fileMenu->addAction("退出(&X)");
    actExit->setShortcut(QKeySequence::Quit);
    connect(actExit, &QAction::triggered, this, &QMainWindow::close);

    // 连接图层变化信号
    connect(layerManager_, &LayerManagerWidget::layerChanged,
            glWidget_, qOverload<>(&QWidget::update));
}

void MainWindow::onOpenFile()
{
    QStringList paths = QFileDialog::getOpenFileNames(
        this, "打开 WKT 文件", {}, "文本文件 (*.txt *.wkt);;所有文件 (*)");
    if (paths.isEmpty()) return;

    try
    {
        for (const QString &path : paths)
        {
            dataengine::WktFileReader reader(path.toStdString());
            auto geometries = reader.read();

            // 从路径提取文件名作为图层名
            QString layerName = QFileInfo(path).fileName();

            // 创建图层
            auto layer = std::make_unique<Layer>(layerName, std::move(geometries));
            layerManager_->addLayer(layer.get());
            layerStore_.push_back(std::move(layer));
        }

        updateLayers();
    }
    catch (const std::exception &e)
    {
        QMessageBox::critical(this, "读取失败", e.what());
    }
}

void MainWindow::onClearAll()
{
    layerStore_.clear();
    layerManager_->clearLayers();
    updateLayers();
}

void MainWindow::updateLayers()
{
    std::vector<Layer *> layerPtrs;
    layerPtrs.reserve(layerStore_.size());
    for (const auto &layer : layerStore_)
        layerPtrs.push_back(layer.get());
    glWidget_->setLayers(std::move(layerPtrs));
}

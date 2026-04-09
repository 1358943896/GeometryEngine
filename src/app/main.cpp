#include "GeometryWidget.h"
#include "dataengine/WktFileReader.h"
#include <QApplication>
#include <QSurfaceFormat>
#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QSpinBox>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>

int main(int argc, char *argv[])
{
    QSurfaceFormat fmt;
    fmt.setVersion(3, 2);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication app(argc, argv);

    QMainWindow mainWin;
    mainWin.setWindowTitle("GeometryViewer");
    mainWin.resize(900, 740);

    auto *glWidget = new GeometryWidget;

    // 中心控件：工具栏 + GL 控件
    auto *central = new QWidget;
    auto *vLayout = new QVBoxLayout(central);
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->setSpacing(0);

    // 工具栏（点大小 / 线宽）
    auto *toolbar  = new QToolBar;
    toolbar->setMovable(false);
    auto *lblPoint = new QLabel("  点大小:");
    auto *sbPoint  = new QSpinBox;
    sbPoint->setRange(1, 50);
    sbPoint->setValue(10);
    auto *lblLine  = new QLabel("  线宽:");
    auto *sbLine   = new QSpinBox;
    sbLine->setRange(1, 20);
    sbLine->setValue(2);
    toolbar->addWidget(lblPoint);
    toolbar->addWidget(sbPoint);
    toolbar->addWidget(lblLine);
    toolbar->addWidget(sbLine);

    vLayout->addWidget(toolbar);
    vLayout->addWidget(glWidget, 1);
    mainWin.setCentralWidget(central);

    // 菜单：文件 → 打开
    auto *fileMenu = mainWin.menuBar()->addMenu("文件(&F)");
    auto *actOpen  = fileMenu->addAction("打开(&O)...");
    actOpen->setShortcut(QKeySequence::Open);

    // 存储当前几何对象（保持生命周期）
    auto *geomStore = new QObject(&mainWin);
    auto *geomList  = new std::vector<std::unique_ptr<geo::Geometry>>;
    geomStore->setProperty("ptr", QVariant::fromValue(static_cast<void *>(geomList)));

    QObject::connect(actOpen, &QAction::triggered, &mainWin,
        [&mainWin, glWidget, geomList]() {
            QString path = QFileDialog::getOpenFileName(
                &mainWin, "打开 WKT 文件", {}, "文本文件 (*.txt *.wkt);;所有文件 (*)");
            if (path.isEmpty()) return;
            try {
                dataengine::WktFileReader reader(path.toStdString());
                *geomList = reader.read();
                std::vector<const geo::Geometry *> ptrs;
                ptrs.reserve(geomList->size());
                for (const auto &g : *geomList)
                    ptrs.push_back(g.get());
                glWidget->setGeometries(std::move(ptrs));
            } catch (const std::exception &e) {
                QMessageBox::critical(&mainWin, "读取失败", e.what());
            }
        });

    QObject::connect(sbPoint, QOverload<int>::of(&QSpinBox::valueChanged),
                     glWidget, &GeometryWidget::setPointSize);
    QObject::connect(sbLine,  QOverload<int>::of(&QSpinBox::valueChanged),
                     glWidget, &GeometryWidget::setLineWidth);

    mainWin.show();
    return QApplication::exec();
}

#pragma once

#include "app/Layer.h"
#include <QMainWindow>
#include <memory>
#include <vector>

class GeometryWidget;
class LayerManagerWidget;

//! \brief 主窗口，管理图层生命周期、菜单和控件布局
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    //! \brief 构造函数
    //! \param parent 父控件
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    //! \brief 打开 WKT 文件并创建新图层
    void onOpenFile();

    //! \brief 清除所有图层
    void onClearAll();

private:
    //! \brief 同步图层列表到 GL 控件并刷新
    void updateLayers();

    GeometryWidget *glWidget_;                          //!< OpenGL 绘制控件
    LayerManagerWidget *layerManager_;                  //!< 图层管理控件
    std::vector<std::unique_ptr<Layer>> layerStore_;   //!< 图层所有权容器
};

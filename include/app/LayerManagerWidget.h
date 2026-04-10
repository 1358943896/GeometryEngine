#pragma once

#include "app/Layer.h"
#include <QWidget>
#include <QTreeWidget>
#include <vector>

//! \brief 图层管理控件，显示图层列表并提供样式编辑功能
class LayerManagerWidget : public QWidget
{
    Q_OBJECT

public:
    //! \brief 构造函数
    //! \param parent 父控件
    explicit LayerManagerWidget(QWidget *parent = nullptr);

    //! \brief 添加图层
    //! \param layer 图层指针（不拥有所有权）
    void addLayer(Layer *layer);

    //! \brief 移除图层
    //! \param layer 图层指针
    void removeLayer(Layer *layer);

    //! \brief 清除所有图层
    void clearLayers();

    //! \brief 返回所有图层
    const std::vector<Layer *> &layers() const;

signals:
    //! \brief 图层变化信号（样式、可见性等），通知 GLWidget 重绘
    void layerChanged();

    //! \brief 图层列表变化信号（添加、删除）
    void layersChanged();

private slots:
    //! \brief 处理图层项可见性变化
    //! \param item 图层项
    //! \param column 列索引
    void onItemChanged(QTreeWidgetItem *item, int column);

    //! \brief 处理图层项双击（展开样式编辑）
    //! \param item 图层项
    //! \param column 列索引
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);

    //! \brief 处理样式变化
    void onStyleChanged();

private:
    //! \brief 创建图层项
    //! \param layer 图层
    //! \return 图层树项
    QTreeWidgetItem *createLayerItem(Layer *layer);

    //! \brief 为图层项添加样式编辑子项
    //! \param parentItem 父项
    //! \param layer 图层
    void addStyleItems(QTreeWidgetItem *parentItem, Layer *layer);

    //! \brief 更新图层项的样式显示
    //! \param item 图层项
    //! \param layer 图层
    void updateStyleDisplay(QTreeWidgetItem *item, Layer *layer);

    //! \brief 从树项获取关联的图层
    //! \param item 树项
    //! \return 图层指针，如果没有返回 nullptr
    Layer *getLayerFromItem(QTreeWidgetItem *item);

    //! \brief 从样式子项获取关联的图层
    //! \param item 样式子项
    //! \return 图层指针，如果没有返回 nullptr
    Layer *getLayerFromStyleItem(QTreeWidgetItem *item);

    QTreeWidget *treeWidget_;          //!< 图层树控件
    std::vector<Layer *> layers_;      //!< 图层列表（不拥有所有权）
};
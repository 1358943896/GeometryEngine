#include "app/LayerManagerWidget.h"

#include <QHeaderView>
#include <QColorDialog>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QTreeWidgetItem>
#include <QInputDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QDoubleSpinBox>

namespace
{
// 自定义树项角色
const int kLayerPtrRole = Qt::UserRole + 1;      //!< 存储 Layer 指针
const int kStyleTypeRole = Qt::UserRole + 2;     //!< 样式类型：0=点颜色,1=点大小,2=线颜色,3=线宽,4=面线颜色,5=面线宽,6=面填充颜色,7=面填充开关
}

LayerManagerWidget::LayerManagerWidget(QWidget *parent)
    : QWidget(parent)
{
    // 创建图层树控件
    treeWidget_ = new QTreeWidget(this);
    treeWidget_->setHeaderLabels(QStringList{QStringLiteral("图层")});
    treeWidget_->header()->setStretchLastSection(true);
    treeWidget_->setAlternatingRowColors(true);
    treeWidget_->setAnimated(true);

    // 连接信号
    connect(treeWidget_, &QTreeWidget::itemChanged,
            this, &LayerManagerWidget::onItemChanged);
    connect(treeWidget_, &QTreeWidget::itemDoubleClicked,
            this, &LayerManagerWidget::onItemDoubleClicked);

    // 设置布局
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(treeWidget_);
}

void LayerManagerWidget::addLayer(Layer *layer)
{
    if (!layer) return;

    layers_.push_back(layer);
    auto *item = createLayerItem(layer);
    treeWidget_->addTopLevelItem(item);
    treeWidget_->expandItem(item);

    emit layersChanged();
}

void LayerManagerWidget::removeLayer(Layer *layer)
{
    if (!layer) return;

    // 从列表中移除
    auto it = std::find(layers_.begin(), layers_.end(), layer);
    if (it != layers_.end())
        layers_.erase(it);

    // 从树中移除对应项
    for (int i = 0; i < treeWidget_->topLevelItemCount(); ++i)
    {
        auto *item = treeWidget_->topLevelItem(i);
        if (getLayerFromItem(item) == layer)
        {
            treeWidget_->takeTopLevelItem(i);
            break;
        }
    }

    emit layersChanged();
}

void LayerManagerWidget::clearLayers()
{
    layers_.clear();
    treeWidget_->clear();
    emit layersChanged();
}

const std::vector<Layer *> &LayerManagerWidget::layers() const
{
    return layers_;
}

void LayerManagerWidget::onItemChanged(QTreeWidgetItem *item, int column)
{
    if (column != 0) return;

    // 判断是图层项还是样式子项：图层项没有父项，样式子项有父项
    bool isLayerItem = (item->parent() == nullptr);

    if (isLayerItem)
    {
        // 图层项：处理可见性复选框
        Layer *layer = getLayerFromItem(item);
        if (layer)
        {
            bool visible = (item->checkState(0) == Qt::Checked);
            // 只有当可见性真正改变时才更新
            if (layer->isVisible() != visible)
            {
                layer->setVisible(visible);
                emit layerChanged();
            }
        }
    }
    else
    {
        // 样式子项：处理填充开关
        int styleType = item->data(0, kStyleTypeRole).toInt();
        if (styleType == 7)  // 填充开关
        {
            Layer *layer = getLayerFromStyleItem(item);
            if (layer)
            {
                bool showFill = (item->checkState(0) == Qt::Checked);
                // 只有当状态真正改变时才更新
                if (layer->regionStyle().showFill != showFill)
                {
                    layer->regionStyle().showFill = showFill;
                    emit layerChanged();
                }
            }
        }
    }
}

void LayerManagerWidget::onItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column)

    // 双击样式子项打开编辑器
    int styleType = item->data(0, kStyleTypeRole).toInt();
    if (styleType < 0) return;

    Layer *layer = getLayerFromStyleItem(item);
    if (!layer) return;

    // 颜色类型：打开颜色选择器
    if (styleType == 0 || styleType == 2 || styleType == 4 || styleType == 6)
    {
        QColor currentColor;
        if (styleType == 0) currentColor = layer->pointStyle().color;
        else if (styleType == 2) currentColor = layer->lineStyle().color;
        else if (styleType == 4) currentColor = layer->regionStyle().lineColor;
        else if (styleType == 6) currentColor = layer->regionStyle().fillColor;

        QColor newColor = QColorDialog::getColor(currentColor, this, QStringLiteral("选择颜色"),
                                                  QColorDialog::ShowAlphaChannel);
        if (newColor.isValid())
        {
            if (styleType == 0) layer->pointStyle().color = newColor;
            else if (styleType == 2) layer->lineStyle().color = newColor;
            else if (styleType == 4) layer->regionStyle().lineColor = newColor;
            else if (styleType == 6) layer->regionStyle().fillColor = newColor;

            updateStyleDisplay(item->parent(), layer);
            emit layerChanged();
        }
    }
    // 数值类型：打开输入对话框
    else if (styleType == 1 || styleType == 3 || styleType == 5)
    {
        float currentValue = 1.0f;
        float minValue = 0.5f;
        float maxValue = 50.0f;
        QString title;

        if (styleType == 1)
        {
            currentValue = layer->pointStyle().size;
            maxValue = 100.0f;
            title = QStringLiteral("设置点大小");
        }
        else if (styleType == 3)
        {
            currentValue = layer->lineStyle().width;
            maxValue = 20.0f;
            title = QStringLiteral("设置线宽");
        }
        else if (styleType == 5)
        {
            currentValue = layer->regionStyle().lineWidth;
            maxValue = 20.0f;
            title = QStringLiteral("设置线宽");
        }

        bool ok = false;
        double newValue = QInputDialog::getDouble(this, title, QStringLiteral("数值:"),
                                                    currentValue, minValue, maxValue, 1, &ok);
        if (ok)
        {
            float value = static_cast<float>(newValue);
            if (styleType == 1) layer->pointStyle().size = value;
            else if (styleType == 3) layer->lineStyle().width = value;
            else if (styleType == 5) layer->regionStyle().lineWidth = value;

            updateStyleDisplay(item->parent(), layer);
            emit layerChanged();
        }
    }
}

void LayerManagerWidget::onStyleChanged()
{
    // 此方法目前未使用，保留以备将来扩展
    // 样式变化现在在 onItemChanged 中直接处理
}

QTreeWidgetItem *LayerManagerWidget::createLayerItem(Layer *layer)
{
    auto *item = new QTreeWidgetItem;
    item->setText(0, layer->name());
    item->setData(0, kLayerPtrRole, QVariant::fromValue(static_cast<void *>(layer)));
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEditable);

    // 暂时阻塞信号，整个创建过程中设置 checkState 会触发 itemChanged
    treeWidget_->blockSignals(true);
    item->setCheckState(0, layer->isVisible() ? Qt::Checked : Qt::Unchecked);

    // 添加样式子项（其中包含 setCheckState 调用）
    addStyleItems(item, layer);

    treeWidget_->blockSignals(false);

    return item;
}

void LayerManagerWidget::addStyleItems(QTreeWidgetItem *parentItem, Layer *layer)
{
    using Type = geo::Geometry::GeometryType;
    Type type = layer->primaryType();

    // 注意：信号阻塞由调用者（createLayerItem）处理

    if (type == Type::Point)
    {
        // 点样式：颜色、点大小
        auto *colorItem = new QTreeWidgetItem(parentItem);
        colorItem->setText(0, QStringLiteral("颜色"));
        colorItem->setData(0, kStyleTypeRole, 0);
        colorItem->setBackground(0, layer->pointStyle().color);

        auto *sizeItem = new QTreeWidgetItem(parentItem);
        sizeItem->setText(0, QStringLiteral("大小: %1").arg(layer->pointStyle().size));
        sizeItem->setData(0, kStyleTypeRole, 1);
    }
    else if (type == Type::Line)
    {
        // 线样式：颜色、线宽
        auto *colorItem = new QTreeWidgetItem(parentItem);
        colorItem->setText(0, QStringLiteral("颜色"));
        colorItem->setData(0, kStyleTypeRole, 2);
        colorItem->setBackground(0, layer->lineStyle().color);

        auto *widthItem = new QTreeWidgetItem(parentItem);
        widthItem->setText(0, QStringLiteral("线宽: %1").arg(layer->lineStyle().width));
        widthItem->setData(0, kStyleTypeRole, 3);
    }
    else // Region
    {
        // 面样式：线颜色、线宽、填充颜色、填充开关
        auto *lineColorItem = new QTreeWidgetItem(parentItem);
        lineColorItem->setText(0, QStringLiteral("线颜色"));
        lineColorItem->setData(0, kStyleTypeRole, 4);
        lineColorItem->setBackground(0, layer->regionStyle().lineColor);

        auto *lineWidthItem = new QTreeWidgetItem(parentItem);
        lineWidthItem->setText(0, QStringLiteral("线宽: %1").arg(layer->regionStyle().lineWidth));
        lineWidthItem->setData(0, kStyleTypeRole, 5);

        auto *fillColorItem = new QTreeWidgetItem(parentItem);
        fillColorItem->setText(0, QStringLiteral("填充颜色"));
        fillColorItem->setData(0, kStyleTypeRole, 6);
        fillColorItem->setBackground(0, layer->regionStyle().fillColor);

        auto *fillToggleItem = new QTreeWidgetItem(parentItem);
        fillToggleItem->setText(0, QStringLiteral("显示填充"));
        fillToggleItem->setCheckState(0, layer->regionStyle().showFill ? Qt::Checked : Qt::Unchecked);
        fillToggleItem->setData(0, kStyleTypeRole, 7);
    }
}

void LayerManagerWidget::updateStyleDisplay(QTreeWidgetItem *item, Layer *layer)
{
    if (!item || !layer) return;

    // 暂时阻塞信号，避免更新时触发 itemChanged
    treeWidget_->blockSignals(true);

    // 更新样式子项显示
    for (int j = 0; j < item->childCount(); ++j)
    {
        auto *styleItem = item->child(j);
        int styleType = styleItem->data(0, kStyleTypeRole).toInt();

        switch (styleType)
        {
        case 0: // 点颜色
            styleItem->setBackground(0, layer->pointStyle().color);
            break;
        case 1: // 点大小
            styleItem->setText(0, QStringLiteral("大小: %1").arg(layer->pointStyle().size));
            break;
        case 2: // 线颜色
            styleItem->setBackground(0, layer->lineStyle().color);
            break;
        case 3: // 线宽
            styleItem->setText(0, QStringLiteral("线宽: %1").arg(layer->lineStyle().width));
            break;
        case 4: // 面线颜色
            styleItem->setBackground(0, layer->regionStyle().lineColor);
            break;
        case 5: // 面线宽
            styleItem->setText(0, QStringLiteral("线宽: %1").arg(layer->regionStyle().lineWidth));
            break;
        case 6: // 面填充颜色
            styleItem->setBackground(0, layer->regionStyle().fillColor);
            break;
        case 7: // 面填充开关
            styleItem->setCheckState(0, layer->regionStyle().showFill ? Qt::Checked : Qt::Unchecked);
            break;
        }
    }

    treeWidget_->blockSignals(false);
}

Layer *LayerManagerWidget::getLayerFromItem(QTreeWidgetItem *item)
{
    if (!item) return nullptr;
    QVariant v = item->data(0, kLayerPtrRole);
    if (!v.isValid()) return nullptr;
    return static_cast<Layer *>(v.value<void *>());
}

Layer *LayerManagerWidget::getLayerFromStyleItem(QTreeWidgetItem *item)
{
    if (!item) return nullptr;
    return getLayerFromItem(item->parent());
}
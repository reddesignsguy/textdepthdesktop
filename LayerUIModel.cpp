#include "LayerUIModel.h"
#include <iostream>

LayerUIModel::LayerUIModel(QObject* parent)
    : QAbstractListModel(parent)
{
    m_layers.push_back({"Background", true, 1.0});
    m_layers.push_back({"Text Layer", true, 1.0});
}

int LayerUIModel::rowCount(const QModelIndex&) const
{
    std::cout << "rowCount = " << m_layers.size() << std::endl;

    return m_layers.size();
}

QVariant LayerUIModel::data(const QModelIndex& index, int role) const
{
    const auto& layer = m_layers[index.row()];

    switch(role)
    {
    case NameRole: return layer.name;
    case VisibleRole: return layer.visible;
    case OpacityRole: return layer.opacity;
    }

    return {};
}

bool LayerUIModel::setData(const QModelIndex& index,
                         const QVariant& value,
                         int role)
{
    auto& layer = m_layers[index.row()];

    switch(role)
    {
    case VisibleRole:
        layer.visible = value.toBool();
        break;

    case NameRole:
        layer.name = value.toString();
        break;

    default:
        return false;
    }

    emit dataChanged(index, index, {role});
    return true;
}

Qt::ItemFlags LayerUIModel::flags(const QModelIndex&) const
{
    return Qt::ItemIsEnabled |
           Qt::ItemIsSelectable |
           Qt::ItemIsEditable;
}

QHash<int,QByteArray> LayerUIModel::roleNames() const
{
    return {
        {NameRole, "layerName"},
        {VisibleRole, "layerVisible"},
        {OpacityRole, "opacity"}
    };
}
void LayerUIModel::addLayer(const QString& name)
{
    std::cout << "adding layer in C++" << std::endl;
    const int row = m_layers.size();

    beginInsertRows(QModelIndex(), row, row);

    m_layers.push_back({
        name,
        true,   // visible
        1.0     // opacity
    });
    for (const auto& layer : m_layers)
        std::cout << layer.name.toStdString()   << std::endl;
    endInsertRows();
}
void LayerUIModel::removeLayer(int row)
{
    if (row < 0 || row >= m_layers.size())
        return;

    beginRemoveRows(QModelIndex(), row, row);

    m_layers.removeAt(row);

    endRemoveRows();
}

#ifndef LAYERUIMODEL_H
#define LAYERUIMODEL_H

#pragma once

#include <QAbstractListModel>

struct LayerUIData
{
    QString name;
    bool visible = true;
    double opacity = 1.0;
};

class LayerUIModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles
    {
        NameRole = Qt::UserRole + 1,
        VisibleRole,
        OpacityRole
    };

    explicit LayerUIModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;

    bool setData(const QModelIndex& index,
                 const QVariant& value,
                 int role) override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void addLayer(const QString& name);
    Q_INVOKABLE void removeLayer(int row);

signals:
    void addLayerSignal(const QString name);

private:
    QVector<LayerUIData> m_layers;
};

#endif // LAYERMODEL_H

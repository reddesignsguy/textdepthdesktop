#ifndef LAYERUIMODEL_H
#define LAYERUIMODEL_H

#include <QAbstractListModel>
#include "TextDepthUnit.h"

// A presentation of backend snapshots. Only row identities, temporary names,
// and selection live here; document data and mutations belong to the backend.
class LayerUIModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int selectedRow READ selectedRow WRITE selectLayer NOTIFY selectedRowChanged)

public:
    enum Roles { NameRole = Qt::UserRole + 1 };

    explicit LayerUIModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QUuid selectedUnitId() const;
    int selectedRow() const { return m_selectedRow; }
    void setUnits(const TextDepthUnits &units);

    Q_INVOKABLE void selectLayer(int row);
    // Requests a move; rows change only when a backend snapshot arrives.
    Q_INVOKABLE bool moveLayer(int from, int to);

signals:
    // Document indices (back-to-front), not display rows (front-to-back).
    void moveRequested(int from, int to);
    void selectedRowChanged();

private:
    struct Row { QUuid unitId; int number; };
    bool isValidRow(int row) const;
    std::vector<Row> m_rows;
    int m_selectedRow = -1;
};

#endif // LAYERUIMODEL_H

#include "LayerUIModel.h"
#include <algorithm>
#include <QSet>

LayerUIModel::LayerUIModel(QObject *parent) : QAbstractListModel(parent) {}

int LayerUIModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

bool LayerUIModel::isValidRow(int row) const
{
    return row >= 0 && row < rowCount();
}

QVariant LayerUIModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.model() != this || index.column() != 0 || !isValidRow(index.row()))
        return {};
    if (role == NameRole || role == Qt::DisplayRole)
        return tr("Layer %1").arg(m_rows[index.row()].number);
    return {};
}

QHash<int, QByteArray> LayerUIModel::roleNames() const
{
    return {{NameRole, "layerName"}};
}

QUuid LayerUIModel::selectedUnitId() const
{
    return isValidRow(m_selectedRow) ? m_rows[m_selectedRow].unitId : QUuid{};
}

void LayerUIModel::setUnits(const TextDepthUnits &units)
{
    const QUuid selection = selectedUnitId();
    const int previousSelection = m_selectedRow;
    QSet<QUuid> existingIds;
    for (const auto &row : m_rows)
        existingIds.insert(row.unitId);
    QSet<QUuid> incomingIds;
    for (const auto &unit : units)
        incomingIds.insert(unit.id);
    const bool sameUnits = units.size() == m_rows.size()
        && incomingIds.size() == units.size() && incomingIds == existingIds;

    if (sameUnits) {
        // Reconcile rows from the authoritative order. Qt move notifications keep
        // delegates and persistent indices attached to their original units.
        for (int to = 0; to < rowCount(); ++to) {
            const auto &id = units[units.size() - 1 - to].id;
            auto source = std::find_if(m_rows.begin() + to, m_rows.end(), [&](const Row &row) {
                return row.unitId == id;
            });
            const int from = static_cast<int>(source - m_rows.begin());
            if (from == to)
                continue;
            beginMoveRows({}, from, from, {}, to);
            std::rotate(m_rows.begin() + to, source, source + 1);
            // Keep selection consistent for observers of rowsMoved as well.
            if (m_selectedRow == from)
                m_selectedRow = to;
            else if (to <= m_selectedRow && m_selectedRow < from)
                ++m_selectedRow;
            endMoveRows();
        }
    } else {
        std::vector<Row> nextRows;
        int nextNumber = 1;
        for (const auto &row : m_rows)
            nextNumber = std::max(nextNumber, row.number + 1);
        const bool anyExisting = std::any_of(units.begin(), units.end(), [&](const auto &unit) {
            return existingIds.contains(unit.id);
        });
        if (!anyExisting)
            nextNumber = 1;
        for (auto unit = units.rbegin(); unit != units.rend(); ++unit) {
            const auto existing = std::find_if(m_rows.begin(), m_rows.end(), [&](const Row &row) {
                return row.unitId == unit->id;
            });
            nextRows.push_back(existing == m_rows.end() ? Row{unit->id, nextNumber++} : *existing);
        }
        beginResetModel();
        m_rows = std::move(nextRows);
        m_selectedRow = -1;
        for (int row = 0; row < rowCount(); ++row) {
            if (m_rows[row].unitId == selection)
                m_selectedRow = row;
        }
        endResetModel();
    }
    if (previousSelection != m_selectedRow || selection != selectedUnitId())
        emit selectedRowChanged();
}

void LayerUIModel::selectLayer(int row)
{
    if (row != -1 && !isValidRow(row))
        return;
    if (row == m_selectedRow)
        return;
    m_selectedRow = row;
    emit selectedRowChanged();
}

bool LayerUIModel::moveLayer(int from, int to)
{
    if (!isValidRow(from) || !isValidRow(to) || from == to)
        return false;
    emit moveRequested(rowCount() - 1 - from, rowCount() - 1 - to);
    return true;
}

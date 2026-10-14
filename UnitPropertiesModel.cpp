#include "UnitPropertiesModel.h"
#include <algorithm>

const UnitPropertiesModel::UnitState *UnitPropertiesModel::selected() const
{
    const auto found = std::find_if(m_states.begin(), m_states.end(), [&](const auto &state) {
        return state.id == m_selection;
    });
    return found == m_states.end() ? nullptr : &*found;
}

bool UnitPropertiesModel::hasSelection() const { return selected() != nullptr; }
bool UnitPropertiesModel::backVisible() const { return selected() && selected()->backVisible; }
bool UnitPropertiesModel::canShowBack() const { return selected() && selected()->canGenerate; }

bool UnitPropertiesModel::canMoveBack() const { return backVisible() && selected()->hasGeometry; }

void UnitPropertiesModel::setUnits(const RenderedTextDepthUnits &units)
{
    m_states.clear();
    for (const auto &unit : units) {
        const auto &path = unit.front.vectorMaskData;
        const auto bounds = path.boundingRect();
        const bool hasGeometry = !path.isEmpty() && bounds.width() > 0 && bounds.height() > 0;
        m_states.push_back({unit.id, unit.back.has_value(), unit.backAvailable || hasGeometry, hasGeometry});
    }
    if (!canMoveBack()) cancelTranslation();
    emit stateChanged();
}

void UnitPropertiesModel::selectUnit(const QUuid &id)
{
    if (m_selection == id)
        return;
    cancelTranslation();
    m_selection = id;
    emit stateChanged();
}

void UnitPropertiesModel::requestToggleBack()
{
    if (!hasSelection() || (!backVisible() && !canShowBack()))
        return;
    emit toggleBackRequested(m_selection);
}

bool UnitPropertiesModel::beginTranslation()
{
    cancelTranslation();
    if (!canMoveBack()) return false;
    m_dragUnit = m_selection;
    emit translationStarted(m_dragUnit);
    return true;
}

void UnitPropertiesModel::translateBack(QPointF delta)
{
    if (!m_dragUnit.isNull()) emit translationChanged(m_dragUnit, delta);
}

void UnitPropertiesModel::finishTranslation()
{
    const auto id = m_dragUnit;
    m_dragUnit = {};
    if (!id.isNull()) emit translationFinished(id);
}

void UnitPropertiesModel::cancelTranslation()
{
    if (m_dragUnit.isNull()) return;
    m_dragUnit = {};
    emit translationCanceled();
}

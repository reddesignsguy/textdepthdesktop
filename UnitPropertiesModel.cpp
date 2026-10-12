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

void UnitPropertiesModel::setUnits(const RenderedTextDepthUnits &units)
{
    m_states.clear();
    for (const auto &unit : units) {
        const auto &path = unit.front.vectorMaskData;
        const auto bounds = path.boundingRect();
        m_states.push_back({unit.id, unit.back.has_value(),
                           unit.backAvailable || (!path.isEmpty() && bounds.width() > 0 && bounds.height() > 0)});
    }
    emit stateChanged();
}

void UnitPropertiesModel::selectUnit(const QUuid &id)
{
    if (m_selection == id)
        return;
    m_selection = id;
    emit stateChanged();
}

void UnitPropertiesModel::requestToggleBack()
{
    if (!hasSelection() || (!backVisible() && !canShowBack()))
        return;
    emit toggleBackRequested(m_selection);
}

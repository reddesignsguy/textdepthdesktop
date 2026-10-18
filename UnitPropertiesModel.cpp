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
        m_states.push_back({unit.id, unit.back.has_value(), unit.backAvailable || hasGeometry, hasGeometry, unit.shadowGradient});
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
    m_selectedStop = {};
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

const GradientStop *UnitPropertiesModel::selectedStop() const
{
    const auto *unit = selected();
    if (!unit || unit->gradient.empty()) return nullptr;
    auto found = std::find_if(unit->gradient.begin(), unit->gradient.end(),
                             [&](const auto &stop) { return stop.id == m_selectedStop; });
    return found == unit->gradient.end() ? &unit->gradient.front() : &*found;
}

QVariantList UnitPropertiesModel::gradientStops() const
{
    QVariantList result;
    if (const auto *unit = selected())
        for (const auto &stop : unit->gradient)
            result.push_back(QVariantMap{{"id", stop.id.toString()}, {"position", stop.position}, {"color", stop.color}});
    return result;
}
QString UnitPropertiesModel::selectedStopId() const { return selectedStop() ? selectedStop()->id.toString() : QString{}; }
QColor UnitPropertiesModel::selectedStopColor() const { return selectedStop() ? selectedStop()->color : QColor(Qt::black); }
double UnitPropertiesModel::selectedStopPosition() const { return selectedStop() ? selectedStop()->position : 0; }

void UnitPropertiesModel::selectStop(const QString &id)
{
    m_selectedStop = QUuid(id);
    emit stateChanged();
}
void UnitPropertiesModel::requestStopColor(QColor color)
{
    if (!selectedStop() || !color.isValid()) return;
    auto stops = selected()->gradient;
    for (auto &stop : stops) if (stop.id.toString() == selectedStopId()) stop.color = color;
    emit shadowGradientRequested(m_selection, std::move(stops));
}
void UnitPropertiesModel::requestStopPosition(double position)
{
    if (!selectedStop()) return;
    auto stops = selected()->gradient;
    for (auto &stop : stops) if (stop.id.toString() == selectedStopId()) stop.position = position;
    // Preserve identity even as sorting moves this stop past others.
    m_selectedStop = selectedStop()->id;
    emit shadowGradientRequested(m_selection, std::move(stops));
}
void UnitPropertiesModel::addStop(double position)
{
    if (!selected()) return;
    auto stops = selected()->gradient;
    GradientStop added{position, sampleShadowGradient(stops, position)};
    stops.push_back(added);
    m_selectedStop = added.id;
    emit shadowGradientRequested(m_selection, std::move(stops));
}
void UnitPropertiesModel::removeSelectedStop()
{
    if (!selectedStop() || selected()->gradient.size() <= 2) return;
    auto stops = selected()->gradient;
    const auto id = selectedStop()->id;
    stops.erase(std::remove_if(stops.begin(), stops.end(), [&](const auto &s) { return s.id == id; }), stops.end());
    m_selectedStop = {};
    emit shadowGradientRequested(m_selection, std::move(stops));
}

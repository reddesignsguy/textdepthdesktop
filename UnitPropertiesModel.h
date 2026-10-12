#ifndef UNITPROPERTIESMODEL_H
#define UNITPROPERTIESMODEL_H

#include <QObject>
#include "TextDepthUnit.h"

// Presentation state only. Mutations are requested by ID and confirmed by a
// backend snapshot, independently of the viewport and layer list.
class UnitPropertiesModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY stateChanged)
    Q_PROPERTY(bool backVisible READ backVisible NOTIFY stateChanged)
    Q_PROPERTY(bool canShowBack READ canShowBack NOTIFY stateChanged)
public:
    explicit UnitPropertiesModel(QObject *parent = nullptr) : QObject(parent) {}
    void setUnits(const RenderedTextDepthUnits &units);
    void selectUnit(const QUuid &id);
    bool hasSelection() const;
    bool backVisible() const;
    bool canShowBack() const;
    Q_INVOKABLE void requestToggleBack();
signals:
    void stateChanged();
    void toggleBackRequested(const QUuid &id);
private:
    struct UnitState { QUuid id; bool backVisible; bool canGenerate; };
    const UnitState *selected() const;
    std::vector<UnitState> m_states;
    QUuid m_selection;
};

#endif

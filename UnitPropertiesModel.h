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
    Q_PROPERTY(bool canMoveBack READ canMoveBack NOTIFY stateChanged)
    Q_PROPERTY(bool canShowBack READ canShowBack NOTIFY stateChanged)
public:
    explicit UnitPropertiesModel(QObject *parent = nullptr) : QObject(parent) {}
    void setUnits(const RenderedTextDepthUnits &units);
    void selectUnit(const QUuid &id);
    bool hasSelection() const;
    bool backVisible() const;
    bool canShowBack() const;
    bool canMoveBack() const;
    Q_INVOKABLE void requestToggleBack();
    Q_INVOKABLE bool beginTranslation();
    Q_INVOKABLE void translateBack(QPointF delta);
    Q_INVOKABLE void finishTranslation();
    Q_INVOKABLE void cancelTranslation();
signals:
    void translationStarted(const QUuid &id);
    void translationChanged(const QUuid &id, QPointF delta);
    void translationFinished(const QUuid &id);
    void translationCanceled();
    void stateChanged();
    void toggleBackRequested(const QUuid &id);
private:
    struct UnitState { QUuid id; bool backVisible; bool canGenerate; bool hasGeometry; };
    const UnitState *selected() const;
    std::vector<UnitState> m_states;
    QUuid m_selection;
    QUuid m_dragUnit;
};

#endif

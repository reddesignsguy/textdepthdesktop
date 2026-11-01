#ifndef UNITPROPERTIESMODEL_H
#define UNITPROPERTIESMODEL_H

#include <QObject>
#include <QVariantList>
#include "TextDepthUnit.h"

// Presentation state only. Mutations are requested by ID and confirmed by a
// backend snapshot, independently of the viewport and layer list.
class UnitPropertiesModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList gradientStops READ gradientStops NOTIFY stateChanged)
    Q_PROPERTY(QString selectedStopId READ selectedStopId NOTIFY stateChanged)
    Q_PROPERTY(QColor selectedStopColor READ selectedStopColor NOTIFY stateChanged)
    Q_PROPERTY(double selectedStopPosition READ selectedStopPosition NOTIFY stateChanged)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY stateChanged)
    Q_PROPERTY(bool backVisible READ backVisible NOTIFY stateChanged)
    Q_PROPERTY(bool canResizeBack READ canMoveBack NOTIFY stateChanged)
    Q_PROPERTY(bool canMoveBack READ canMoveBack NOTIFY stateChanged)
    Q_PROPERTY(bool canShowBack READ canShowBack NOTIFY stateChanged)
public:
    explicit UnitPropertiesModel(QObject *parent = nullptr) : QObject(parent) {}
    void setUnits(const RenderedTextDepthUnits &units);
    void selectUnit(const QUuid &id);
    QVariantList gradientStops() const;
    QString selectedStopId() const;
    QColor selectedStopColor() const;
    double selectedStopPosition() const;
    Q_INVOKABLE void selectStop(const QString &id);
    Q_INVOKABLE void requestStopColor(QColor color);
    Q_INVOKABLE void requestStopPosition(double position);
    Q_INVOKABLE void addStop(double position);
    Q_INVOKABLE void removeSelectedStop();
    bool hasSelection() const;
    bool backVisible() const;
    bool canShowBack() const;
    bool canMoveBack() const;
    Q_INVOKABLE void requestToggleBack();
    Q_INVOKABLE bool beginTranslation();
    Q_INVOKABLE void translateBack(QPointF delta);
    Q_INVOKABLE void finishTranslation();
    Q_INVOKABLE void cancelTranslation();
    Q_INVOKABLE bool beginResize();
    Q_INVOKABLE void resizeBack(QPointF delta);
    Q_INVOKABLE void finishResize();
    Q_INVOKABLE void cancelResize();
signals:
    void resizeStarted(const QUuid &id);
    void resizeChanged(const QUuid &id, QPointF delta);
    void resizeFinished(const QUuid &id);
    void resizeCanceled();
    void shadowGradientRequested(const QUuid &id, ShadowGradient stops);
    void translationStarted(const QUuid &id);
    void translationChanged(const QUuid &id, QPointF delta);
    void translationFinished(const QUuid &id);
    void translationCanceled();
    void stateChanged();
    void toggleBackRequested(const QUuid &id);
private:
    struct UnitState { QUuid id; bool backVisible; bool canGenerate; bool hasGeometry; ShadowGradient gradient; };
    const UnitState *selected() const;
    std::vector<UnitState> m_states;
    QUuid m_selection;
    QUuid m_dragUnit;
    QUuid m_resizeUnit;
    QUuid m_selectedStop;
    const GradientStop *selectedStop() const;
};

#endif

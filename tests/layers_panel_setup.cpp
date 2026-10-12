#include "LayerUIModel.h"
#include "TextDepth.h"
#include "UnitPropertiesModel.h"
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QtQuickTest/quicktest.h>

class LayersPanelSetup : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE void resetUnits(int count = 3) {
        TextDepthUnits units(count);
        for (auto &unit : units)
            unit.front.vectorMaskData.addRect(10, 10, 30, 30);
        m_backend.setUnits(std::move(units));
    }
public slots:
    void qmlEngineAvailable(QQmlEngine *engine)
    {
        connect(&m_backend, &TextDepth::notifyNewQtData, &m_model, &LayerUIModel::setUnits);
        connect(&m_model, &LayerUIModel::moveRequested, &m_backend, &TextDepth::moveUnit);
        connect(&m_backend, &TextDepth::notifyNewQtData, &m_properties, &UnitPropertiesModel::setUnits);
        connect(&m_model, &LayerUIModel::selectedRowChanged, &m_properties, [&] {
            m_properties.selectUnit(m_model.selectedUnitId());
        });
        connect(&m_properties, &UnitPropertiesModel::toggleBackRequested, &m_backend, &TextDepth::handleToggleUnitVisibility);
        engine->rootContext()->setContextProperty("documentLayerModel", &m_model);
        engine->rootContext()->setContextProperty("unitPropertiesModel", &m_properties);
        engine->rootContext()->setContextProperty("textDepthBackend", &m_backend);
        auto *viewport = new QQuickItem;
        viewport->setParent(engine);
        engine->rootContext()->setContextProperty("textDepthWidget", viewport);
        engine->rootContext()->setContextProperty("testProperties", &m_properties);
        engine->rootContext()->setContextProperty("testLayerModel", &m_model);
        engine->rootContext()->setContextProperty("testSetup", this);
    }
private:
    TextDepth m_backend{64, 256};
    LayerUIModel m_model;
    UnitPropertiesModel m_properties;
};

QUICK_TEST_MAIN_WITH_SETUP(layers_panel, LayersPanelSetup)
#include "layers_panel_setup.moc"

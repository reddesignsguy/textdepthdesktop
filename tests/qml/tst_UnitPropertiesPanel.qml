import QtQuick
import QtTest
import "../.."

TestCase {
    name: "UnitPropertiesPanel"
    width: 400
    height: 300
    visible: true
    when: windowShown

    UnitPropertiesPanel {
        id: panel
        width: 300
        propertiesModel: testProperties
    }

    function init() {
        testSetup.resetUnits(2)
        waitForRendering(panel)
    }

    function test_toggleSelectionAndReorder() {
        const toggle = findChild(panel, "backToggle")
        verify(!testProperties.hasSelection)
        verify(!toggle.enabled)
        testLayerModel.selectLayer(0)
        waitForRendering(panel)
        verify(toggle.enabled)
        mouseClick(toggle)
        verify(testProperties.backVisible)
        compare(toggle.text, "Hide back")
        testLayerModel.selectLayer(1)
        verify(!testProperties.backVisible)
        testLayerModel.selectLayer(0)
        verify(testProperties.backVisible)
        testLayerModel.moveLayer(0, 1)
        verify(testProperties.backVisible)
        mouseClick(toggle)
        verify(!testProperties.backVisible)
        testSetup.resetUnits(0)
        verify(!testProperties.hasSelection)
        verify(!toggle.enabled)
    }
}

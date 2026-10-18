import QtQuick
import QtTest
import "../.."

TestCase {
    name: "UnitPropertiesPanel"
    width: 400
    height: 800
    visible: running
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

    function test_liveGradientEditor() {
        testLayerModel.selectLayer(0)
        waitForRendering(panel)
        const original = testProperties.selectedStopColor.toString()
        const square = findChild(panel, "saturationBrightnessSquare")
        verify(square !== null)
        compare(square.width, square.height)
        mousePress(square, square.width * 0.8, square.height * 0.2)
        verify(testProperties.selectedStopColor.toString() !== original)
        const firstColor = testProperties.selectedStopColor.toString()
        mouseMove(square, square.width * 0.3, square.height * 0.7)
        verify(testProperties.selectedStopColor.toString() !== firstColor, "preview changes before releasing")
        mouseRelease(square, square.width * 0.3, square.height * 0.7)
        compare(testProperties.selectedStopColor.a, 1)
        const hue = findChild(panel, "hueSlider")
        const beforeHue = testProperties.selectedStopColor.toString()
        mousePress(hue, hue.width * 0.6, hue.height / 2)
        mouseMove(hue, hue.width * 0.7, hue.height / 2)
        verify(testProperties.selectedStopColor.toString() !== beforeHue, "hue updates before releasing")
        mouseRelease(hue, hue.width * 0.7, hue.height / 2)
        const edited = testProperties.selectedStopColor.toString()
        testLayerModel.selectLayer(1)
        compare(testProperties.selectedStopColor.toString(), original)
        testLayerModel.selectLayer(0)
        compare(testProperties.selectedStopColor.toString(), edited)
        mouseClick(findChild(panel, "addGradientStop"))
        compare(testProperties.gradientStops.length, 3)
        const id = testProperties.selectedStopId
        const position = findChild(panel, "gradientStopPosition")
        mouseDrag(position, position.width / 2, position.height / 2, position.width * 0.3, 0)
        verify(testProperties.selectedStopPosition > 0.5)
        compare(testProperties.selectedStopId, id)
        mouseClick(findChild(panel, "removeGradientStop"))
        compare(testProperties.gradientStops.length, 2)
        verify(!findChild(panel, "removeGradientStop").enabled)
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

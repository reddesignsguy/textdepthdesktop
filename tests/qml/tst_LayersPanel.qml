import QtQuick
import QtTest
import "../.."

TestCase {
    name: "LayersPanel"
    width: 400
    height: 380
    when: windowShown
    visible: true

    LayersPanel {
        id: panel
        width: 300
        height: 320
        layerModel: testLayerModel
    }

    function init() {
        testSetup.resetUnits(3)
        panel.cancelDrag()
        waitForRendering(panel)
    }

    function rowY(index) { return 36 + index * 44 + 22 }

    function test_selection() {
        mouseClick(panel, 40, rowY(0))
        compare(testLayerModel.selectedRow, 0)
        mouseClick(panel, 40, rowY(2), Qt.LeftButton, Qt.ControlModifier)
        compare(testLayerModel.selectedRow, 2)
    }

    function test_dragDownAndUp() {
        mousePress(panel, 40, rowY(0))
        mouseMove(panel, 40, rowY(2), 30)
        verify(panel.dragging)
        compare(panel.dragTo, 2)
        mouseRelease(panel, 40, rowY(2))
        compare(testLayerModel.selectedRow, 2)
        compare(testLayerModel.data(testLayerModel.index(2, 0), 257), "Layer 1")
        mousePress(panel, 40, rowY(2))
        mouseMove(panel, 40, rowY(0), 30)
        mouseRelease(panel, 40, rowY(0))
        compare(testLayerModel.selectedRow, 0)
        compare(testLayerModel.data(testLayerModel.index(0, 0), 257), "Layer 1")
    }

    function test_cancelAndOutsideDrop() {
        mousePress(panel, 40, rowY(0))
        mouseMove(panel, 40, rowY(2), 30)
        keyClick(Qt.Key_Escape)
        mouseRelease(panel, 40, rowY(2))
        compare(testLayerModel.data(testLayerModel.index(0, 0), 257), "Layer 1")
        mousePress(panel, 40, rowY(0))
        mouseMove(panel, 350, rowY(2), 30)
        mouseRelease(panel, 350, rowY(2))
        compare(testLayerModel.data(testLayerModel.index(0, 0), 257), "Layer 1")
    }

    function test_emptyAndReplacement() {
        mouseClick(panel, 40, rowY(1))
        testSetup.resetUnits(0)
        compare(testLayerModel.selectedRow, -1)
        compare(findChild(panel, "layerList").count, 0)
        testSetup.resetUnits(1)
        waitForRendering(panel)
        compare(testLayerModel.selectedRow, -1)
        mouseClick(panel, 40, rowY(0))
        compare(testLayerModel.selectedRow, 0)
    }

    function test_edgeScrolling() {
        testSetup.resetUnits(20)
        waitForRendering(panel)
        const list = findChild(panel, "layerList")
        mousePress(panel, 40, rowY(0))
        mouseMove(panel, 40, 36 + list.height - 10, 30)
        tryVerify(function() { return list.contentY > 0 })
        mouseRelease(panel, 40, 36 + list.height - 10)
        verify(testLayerModel.selectedRow > 0)
    }
}

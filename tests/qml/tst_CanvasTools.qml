import QtQuick
import QtTest
import "../.."

TestCase {
    name: "CanvasTools"
    width: 500
    height: 400
    visible: running
    when: windowShown
    CanvasToolBar { id: toolbar; width: parent.width }
    Item { id: testScene; x: 0; y: 0; scale: 2; transformOrigin: Item.TopLeft }
    NavigationTool {
        id: navigation
        y: 60; width: 400; height: 300
        scene: testScene
        enabled: toolbar.activeTool === "navigate"
        visible: enabled
    }
    BackMoveTool {
        id: moveTool
        y: 60; width: 400; height: 300
        scene: testScene
        propertiesModel: testProperties
        enabled: toolbar.activeTool === "backMove"
        visible: enabled
    }
    BackResizeTool {
        id: resizeTool
        y: 60; width: 400; height: 300
        scene: testScene
        propertiesModel: testProperties
        enabled: toolbar.activeTool === "backResize"
        visible: enabled
    }
    SignalSpy { id: translations; target: testProperties; signalName: "translationChanged" }
    SignalSpy { id: finished; target: testProperties; signalName: "translationFinished" }
    SignalSpy { id: canceled; target: testProperties; signalName: "translationCanceled" }
    SignalSpy { id: resizes; target: testProperties; signalName: "resizeChanged" }
    SignalSpy { id: resizeFinished; target: testProperties; signalName: "resizeFinished" }
    SignalSpy { id: resizeCanceled; target: testProperties; signalName: "resizeCanceled" }
    function init() {
        toolbar.activeTool = "navigate"
        testScene.x = 0; testScene.y = 0; testScene.scale = 2
        testSetup.resetUnits(2)
        translations.clear(); finished.clear(); canceled.clear()
        resizes.clear(); resizeFinished.clear(); resizeCanceled.clear()
    }
    function test_navigationAndExclusiveTools() {
        const nav = findChild(toolbar, "navigateToolButton")
        const back = findChild(toolbar, "backMoveToolButton")
        verify(nav.checked && !back.checked)
        mouseDrag(navigation, 100, 100, 20, 30, Qt.LeftButton)
        compare(testScene.x, 20); compare(testScene.y, 30)
        mouseClick(back)
        verify(back.checked && !nav.checked)
        compare(moveTool.cursorShape, Qt.ForbiddenCursor)
        mouseClick(back)
        verify(back.checked)
    }
    function test_dragUsesDocumentCoordinatesAndCancels() {
        testLayerModel.selectLayer(0)
        testProperties.requestToggleBack()
        toolbar.activeTool = "backMove"
        waitForRendering(moveTool)
        mouseDrag(moveTool, 100, 100, 40, 20, Qt.LeftButton)
        compare(finished.count, 1)
        // Signal carries ID first and document-space delta second.
        const point = translations.signalArguments[translations.count - 1][1]
        compare(point.x, 20); compare(point.y, 10)
        compare(testScene.x, 0); compare(testScene.y, 0)
        mousePress(moveTool, 100, 100)
        mouseMove(moveTool, 120, 120)
        keyClick(Qt.Key_Escape)
        compare(canceled.count, 1)
        mouseRelease(moveTool, 120, 120)
        compare(finished.count, 1)
        mousePress(moveTool, 100, 100)
        toolbar.activeTool = "navigate"
        compare(canceled.count, 2)
        mouseRelease(navigation, 100, 100)
    }
    function test_resizeUsesDocumentCoordinatesAndCancels() {
        testLayerModel.selectLayer(0)
        testProperties.requestToggleBack()
        toolbar.activeTool = "backResize"
        waitForRendering(resizeTool)
        mouseDrag(resizeTool, 100, 100, 40, 20, Qt.LeftButton)
        compare(resizeFinished.count, 1)
        // Signal carries ID first and document-space delta second.
        const point = resizes.signalArguments[resizes.count - 1][1]
        compare(point.x, 20); compare(point.y, 10)
        compare(testScene.x, 0); compare(testScene.y, 0)
        mousePress(resizeTool, 100, 100)
        mouseMove(resizeTool, 120, 120)
        keyClick(Qt.Key_Escape)
        compare(resizeCanceled.count, 1)
        mouseRelease(resizeTool, 120, 120)
        compare(resizeFinished.count, 1)
        mousePress(resizeTool, 100, 100)
        toolbar.activeTool = "navigate"
        compare(resizeCanceled.count, 2)
        mouseRelease(navigation, 100, 100)
    }
}

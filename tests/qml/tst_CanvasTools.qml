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
    ToolOptionsBar {
        id: options
        y: toolbar.height
        width: parent.width
        activeTool: toolbar.activeTool
    }
    Item { id: testScene; x: 0; y: 0; scale: 2; transformOrigin: Item.TopLeft }
    NavigationTool {
        id: navigation
        y: 100; width: 400; height: 300
        scene: testScene
        enabled: toolbar.activeTool === "navigate"
        visible: enabled
    }
    BackMoveTool {
        id: moveTool
        y: 100; width: 400; height: 300
        scene: testScene
        propertiesModel: testProperties
        verticalOnly: options.moveVerticalOnly
        enabled: toolbar.activeTool === "backMove"
        visible: enabled
    }
    BackResizeTool {
        id: resizeTool
        y: 100; width: 400; height: 300
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
        options.moveVerticalOnly = false
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
    function test_verticalOnlyMovement() {
        verify(!options.visible)
        testLayerModel.selectLayer(0)
        testProperties.requestToggleBack()
        toolbar.activeTool = "backMove"
        verify(options.visible)
        const toggle = findChild(options, "moveVerticalOnlyCheckBox")
        mouseClick(toggle)
        verify(moveTool.verticalOnly)
        compare(moveTool.cursorShape, Qt.SizeVerCursor)
        waitForRendering(moveTool)
        mousePress(moveTool, 100, 100)
        mouseMove(moveTool, 140, 80)
        verify(translations.count > 0)
        for (let i = 0; i < translations.count; ++i)
            compare(translations.signalArguments[i][1].x, 0)
        mouseRelease(moveTool, 160, 60)
        compare(finished.count, 1)
        const point = translations.signalArguments[translations.count - 1][1]
        compare(point.x, 0); compare(point.y, -20)

        toolbar.activeTool = "backResize"
        verify(!options.visible)
        toolbar.activeTool = "backMove"
        verify(toggle.checked)
        mouseClick(toggle)
        mouseDrag(moveTool, 100, 100, 40, 20, Qt.LeftButton)
        const freePoint = translations.signalArguments[translations.count - 1][1]
        compare(freePoint.x, 20); compare(freePoint.y, 10)
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

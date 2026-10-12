import QtQuick
import QtTest

TestCase {
    name: "MainLayout"
    when: windowShown

    function test_sidebarLeavesViewportSpace() {
        const component = Qt.createComponent("../../Main.qml")
        compare(component.status, Component.Ready, component.errorString())
        const window = component.createObject(null, { visible: true, width: 960, height: 720 })
        verify(window !== null)
        const viewport = findChild(window, "documentViewport")
        const sidebar = findChild(window, "editorSidebar")
        tryCompare(sidebar, "width", 300)
        tryVerify(function() { return viewport.width > 600 }, 2000,
                  "Viewport width: " + viewport.width + ", row width: " + viewport.parent.width)
        testSetup.resetUnits(2)
        testLayerModel.selectLayer(0)
        wait(30)
        compare(sidebar.width, 300)
        verify(viewport.width > 600, "Selection must not expand the sidebar")
        window.width = 640
        tryCompare(sidebar, "width", 256)
        tryVerify(function() { return viewport.width > 370 }, 2000, "Narrow viewport width: " + viewport.width)
        window.close()
        window.destroy()
    }
}

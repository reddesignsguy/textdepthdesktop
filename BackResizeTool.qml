import QtQuick

MouseArea {
    id: tool
    required property Item scene
    required property var propertiesModel
    property point origin
    property bool dragging: false
    acceptedButtons: Qt.LeftButton
    cursorShape: propertiesModel.canResizeBack ? Qt.SizeFDiagCursor : Qt.ForbiddenCursor
    focus: true
    function cancel() {
        if (dragging) propertiesModel.cancelResize()
        dragging = false
    }
    onPressed: function(mouse) {
        forceActiveFocus()
        dragging = propertiesModel.beginResize()
        origin = mapToItem(scene, mouse.x, mouse.y)
    }
    onPositionChanged: function(mouse) {
        if (!dragging) return
        const point = mapToItem(scene, mouse.x, mouse.y)
        propertiesModel.resizeBack(Qt.point(point.x - origin.x, point.y - origin.y))
    }
    onReleased: function(mouse) {
        if (!dragging) return
        const point = mapToItem(scene, mouse.x, mouse.y)
        propertiesModel.resizeBack(Qt.point(point.x - origin.x, point.y - origin.y))
        propertiesModel.finishResize()
        dragging = false
    }
    onCanceled: cancel()
    onEnabledChanged: { if (!enabled) cancel() }
    Component.onDestruction: cancel()
    Keys.onEscapePressed: cancel()
    Connections {
        target: tool.propertiesModel
        function onResizeCanceled() { tool.dragging = false }
    }
}

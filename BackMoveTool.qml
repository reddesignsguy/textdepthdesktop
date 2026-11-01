import QtQuick

MouseArea {
    id: tool
    required property Item scene
    required property var propertiesModel
    property point origin
    property bool dragging: false
    property bool verticalOnly: false
    acceptedButtons: Qt.LeftButton
    cursorShape: propertiesModel.canMoveBack
        ? (verticalOnly ? Qt.SizeVerCursor : Qt.SizeAllCursor) : Qt.ForbiddenCursor
    focus: true
    function translationAt(x, y) {
        const point = mapToItem(scene, x, y)
        return Qt.point(verticalOnly ? 0 : point.x - origin.x, point.y - origin.y)
    }
    function cancel() {
        if (dragging) propertiesModel.cancelTranslation()
        dragging = false
    }
    onPressed: function(mouse) {
        forceActiveFocus()
        dragging = propertiesModel.beginTranslation()
        origin = mapToItem(scene, mouse.x, mouse.y)
    }
    onPositionChanged: function(mouse) {
        if (!dragging) return
        propertiesModel.translateBack(translationAt(mouse.x, mouse.y))
    }
    onReleased: function(mouse) {
        if (!dragging) return
        propertiesModel.translateBack(translationAt(mouse.x, mouse.y))
        propertiesModel.finishTranslation()
        dragging = false
    }
    onCanceled: cancel()
    onEnabledChanged: { if (!enabled) cancel() }
    Component.onDestruction: cancel()
    Keys.onEscapePressed: cancel()
    Connections {
        target: tool.propertiesModel
        function onTranslationCanceled() { tool.dragging = false }
    }
}

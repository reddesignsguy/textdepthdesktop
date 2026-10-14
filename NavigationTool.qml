import QtQuick

Item {
    id: tool
    required property Item scene
    function zoomAt(point, factor) {
        const oldScale = scene.scale
        const nextScale = Math.max(0.05, Math.min(20, oldScale * factor))
        const ratio = nextScale / oldScale
        scene.x = point.x - (point.x - scene.x) * ratio
        scene.y = point.y - (point.y - scene.y) * ratio
        scene.scale = nextScale
    }
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.MiddleButton
        cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
        property point previous
        onPressed: function(mouse) { previous = Qt.point(mouse.x, mouse.y) }
        onPositionChanged: function(mouse) {
            if (!pressed) return
            tool.scene.x += mouse.x - previous.x
            tool.scene.y += mouse.y - previous.y
            previous = Qt.point(mouse.x, mouse.y)
        }
    }
    WheelHandler {
        target: null
        onWheel: function(event) {
            tool.zoomAt(Qt.point(event.x, event.y), Math.pow(1.1, event.angleDelta.y / 120))
        }
    }
    PinchHandler {
        target: null
        property real previousScale: 1
        onActiveChanged: previousScale = 1
        onActiveScaleChanged: {
            if (!active) return
            tool.zoomAt(centroid.position, activeScale / previousScale)
            previousScale = activeScale
        }
    }
}

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: panel
    required property var layerModel
    color: "#2B2B2B"

    property int dragFrom: -1
    property int dragTo: -1
    property bool dragging: false
    property point pressPoint
    property point pointerPosition

    function cancelDrag() {
        dragFrom = -1
        dragTo = -1
        dragging = false
    }

    function updateTarget() {
        if (pointerPosition.x < 0 || pointerPosition.x > layerView.width
                || pointerPosition.y < 0 || pointerPosition.y > layerView.height) {
            dragTo = -1
            return
        }
        dragTo = layerView.indexAt(pointerPosition.x + layerView.contentX,
                                   pointerPosition.y + layerView.contentY)
    }

    function trackPointer(point, threshold) {
        pointerPosition = point
        if (!dragging && Math.hypot(point.x - pressPoint.x, point.y - pressPoint.y)
                >= threshold)
            dragging = true
        if (dragging)
            updateTarget()
    }

    Keys.onEscapePressed: cancelDrag()

    Connections {
        target: panel.layerModel
        function onModelAboutToBeReset() { panel.cancelDrag() }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Label {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            text: qsTr("Layers")
            color: "white"
            font.bold: true
            leftPadding: 12
            verticalAlignment: Text.AlignVCenter
            background: Rectangle { color: "#353535" }
        }

        ListView {
            id: layerView
            objectName: "layerList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: panel.layerModel
            currentIndex: panel.layerModel.selectedRow
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}

            Label {
                anchors.centerIn: parent
                visible: layerView.count === 0
                text: qsTr("Open a PSD to see its layers")
                color: "#AAAAAA"
            }

            delegate: Rectangle {
                id: layerRow
                required property int index
                required property string layerName
                width: layerView.width
                height: 44
                color: panel.layerModel.selectedRow === index ? "#4A6EA8" : "#3A3A3A"
                border.color: "#555555"
                opacity: panel.dragging && panel.dragFrom === index ? 0.55 : 1
                Accessible.role: Accessible.ListItem
                Accessible.name: layerName
                Accessible.selected: panel.layerModel.selectedRow === index
                Accessible.onPressAction: panel.layerModel.selectLayer(index)

                Label {
                    anchors.fill: parent
                    leftPadding: 12
                    rightPadding: 32
                    verticalAlignment: Text.AlignVCenter
                    text: layerRow.layerName
                    color: "white"
                    elide: Text.ElideRight
                }
                Label {
                    anchors.right: parent.right
                    anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    text: "≡"
                    color: "#BBBBBB"
                }
                // Show the insertion edge without mutating the document until release.
                Rectangle {
                    width: parent.width
                    height: 3
                    y: panel.dragTo > panel.dragFrom ? parent.height - height : 0
                    color: "#8CB8FF"
                    visible: panel.dragging && panel.dragTo === layerRow.index
                             && panel.dragFrom !== panel.dragTo
                }
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    preventStealing: true
                    cursorShape: pressed && panel.dragging ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                    onPressed: function(mouse) {
                        panel.forceActiveFocus()
                        panel.layerModel.selectLayer(layerRow.index)
                        panel.dragFrom = layerRow.index
                        panel.pressPoint = mapToItem(layerView, mouse.x, mouse.y)
                        panel.pointerPosition = panel.pressPoint
                    }
                    onPositionChanged: function(mouse) {
                        if (pressed && panel.dragFrom >= 0)
                            panel.trackPointer(mapToItem(layerView, mouse.x, mouse.y), drag.threshold)
                    }
                    onReleased: function(mouse) {
                        if (panel.dragging) {
                            panel.pointerPosition = mapToItem(layerView, mouse.x, mouse.y)
                            panel.updateTarget()
                            panel.layerModel.moveLayer(panel.dragFrom, panel.dragTo)
                        }
                        panel.cancelDrag()
                    }
                    onCanceled: panel.cancelDrag()
                }
            }
        }

        Label {
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            text: qsTr("Drag to reorder · Top layer is in front")
            color: "#AAAAAA"
            font.pixelSize: 11
            leftPadding: 10
            verticalAlignment: Text.AlignVCenter
        }
    }

    // Keep distant rows reachable while dragging through a long document.
    Timer {
        interval: 30
        repeat: true
        running: panel.dragging
        onTriggered: {
            const y = panel.pointerPosition.y
            if (panel.pointerPosition.x < 0 || panel.pointerPosition.x > layerView.width)
                return
            const step = y < 28 ? -10 : (y > layerView.height - 28 ? 10 : 0)
            const minimum = layerView.originY
            const maximum = minimum + Math.max(0, layerView.contentHeight - layerView.height)
            layerView.contentY = Math.max(minimum, Math.min(maximum, layerView.contentY + step))
            panel.updateTarget()
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: panel
    required property var propertiesModel
    spacing: 8
    Label { text: qsTr("Back gradient"); color: "white"; font.bold: true }
    Label {
        Layout.fillWidth: true
        text: qsTr("Select or drag a stop. Double-click to add one.")
        wrapMode: Text.WordWrap
        color: "#AAAAAA"
    }
    Item {
        id: strip
        objectName: "gradientStrip"
        Layout.fillWidth: true
        implicitHeight: 38
        Canvas {
            id: gradientCanvas
            anchors.fill: parent
            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                const gradient = ctx.createLinearGradient(0, 0, width, 0)
                const stops = panel.propertiesModel.gradientStops
                for (let i = 0; i < stops.length; ++i)
                    gradient.addColorStop(stops[i].position, stops[i].color.toString())
                ctx.fillStyle = gradient
                ctx.fillRect(0, 0, width, 24)
            }
            Connections {
                target: panel.propertiesModel
                function onStateChanged() { gradientCanvas.requestPaint() }
            }
        }
        Repeater {
            model: panel.propertiesModel.gradientStops
            Rectangle {
                required property var modelData
                x: Math.max(0, Math.min(strip.width - width, modelData.position * strip.width - width / 2))
                y: 24
                width: 12; height: 14; radius: 2
                color: modelData.color
                border.color: modelData.id === panel.propertiesModel.selectedStopId ? "white" : "#777777"
                border.width: modelData.id === panel.propertiesModel.selectedStopId ? 2 : 1
            }
        }
        MouseArea {
            anchors.fill: parent
            // A stop drag owns the pointer until release, even outside the strip.
            preventStealing: true
            function positionAt(mouse) { return Math.max(0, Math.min(1, mouse.x / width)) }
            onPressed: function(mouse) {
                const stops = panel.propertiesModel.gradientStops
                let closest = 0
                for (let i = 1; i < stops.length; ++i)
                    if (Math.abs(stops[i].position - positionAt(mouse)) < Math.abs(stops[closest].position - positionAt(mouse))) closest = i
                if (stops.length) panel.propertiesModel.selectStop(stops[closest].id)
            }
            onPositionChanged: function(mouse) {
                if (pressed) panel.propertiesModel.requestStopPosition(positionAt(mouse))
            }
            onDoubleClicked: function(mouse) { panel.propertiesModel.addStop(positionAt(mouse)) }
        }
    }
    RowLayout {
        Layout.fillWidth: true
        Button {
            objectName: "addGradientStop"
            text: qsTr("Add stop")
            Layout.fillWidth: true
            onClicked: {
                const stops = panel.propertiesModel.gradientStops
                let start = 0, end = 1, gap = -1
                for (let i = 0; i < stops.length - 1; ++i) {
                    const size = stops[i + 1].position - stops[i].position
                    if (size > gap) { gap = size; start = stops[i].position; end = stops[i + 1].position }
                }
                panel.propertiesModel.addStop((start + end) / 2)
            }
        }
        Button {
            objectName: "removeGradientStop"
            text: qsTr("Remove")
            Layout.fillWidth: true
            enabled: panel.propertiesModel.gradientStops.length > 2
            onClicked: panel.propertiesModel.removeSelectedStop()
        }
    }
    Label {
        color: "#CCCCCC"
        text: qsTr("Position: %1%").arg(Math.round(panel.propertiesModel.selectedStopPosition * 100))
    }
    Slider {
        objectName: "gradientStopPosition"
        Layout.fillWidth: true
        from: 0; to: 1
        value: panel.propertiesModel.selectedStopPosition
        Accessible.name: qsTr("Gradient stop position")
        onMoved: panel.propertiesModel.requestStopPosition(value)
    }
    RgbColorControl {
        objectName: "gradientStopColor"
        Layout.fillWidth: true
        value: panel.propertiesModel.selectedStopColor
        onColorRequested: function(color) { panel.propertiesModel.requestStopColor(color) }
    }
}

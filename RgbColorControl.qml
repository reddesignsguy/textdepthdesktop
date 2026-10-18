import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: control
    required property color value
    property real hue: 0
    signal colorRequested(color color)
    spacing: 6
    onValueChanged: {
        if (value.hsvHue >= 0 && !hueSlider.pressed && !squareInput.pressed)
            hue = value.hsvHue
    }
    Component.onCompleted: { if (value.hsvHue >= 0) hue = value.hsvHue }

    Rectangle {
        id: square
        objectName: "saturationBrightnessSquare"
        Layout.preferredWidth: Math.min(control.width, 180)
        Layout.preferredHeight: Math.min(control.width, 180)
        Layout.alignment: Qt.AlignHCenter
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0; color: "white" }
            GradientStop { position: 1; color: Qt.hsva(control.hue, 1, 1, 1) }
        }
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0; color: "transparent" }
                GradientStop { position: 1; color: "black" }
            }
        }
        Rectangle {
            width: 10; height: 10; radius: 5
            x: control.value.hsvSaturation * square.width - width / 2
            y: (1 - control.value.hsvValue) * square.height - height / 2
            color: "transparent"; border.color: "white"; border.width: 2
        }
        MouseArea {
            id: squareInput
            anchors.fill: parent
            // Keep color drags from turning into a parent ScrollView flick.
            preventStealing: true
            cursorShape: Qt.CrossCursor
            function choose(mouse) {
                const saturation = Math.max(0, Math.min(1, mouse.x / width))
                const brightness = 1 - Math.max(0, Math.min(1, mouse.y / height))
                control.colorRequested(Qt.hsva(control.hue, saturation, brightness, 1))
            }
            onPressed: function(mouse) { choose(mouse) }
            onPositionChanged: function(mouse) { if (pressed) choose(mouse) }
        }
    }
    Slider {
        id: hueSlider
        objectName: "hueSlider"
        Layout.fillWidth: true
        from: 0; to: 1
        value: control.hue
        Accessible.name: qsTr("Hue")
        background: Rectangle {
            x: hueSlider.leftPadding
            y: hueSlider.topPadding + (hueSlider.availableHeight - height) / 2
            width: hueSlider.availableWidth; height: 14
            radius: 3
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0; color: "#ff0000" }
                GradientStop { position: 0.1667; color: "#ffff00" }
                GradientStop { position: 0.3333; color: "#00ff00" }
                GradientStop { position: 0.5; color: "#00ffff" }
                GradientStop { position: 0.6667; color: "#0000ff" }
                GradientStop { position: 0.8333; color: "#ff00ff" }
                GradientStop { position: 1; color: "#ff0000" }
            }
        }
        onMoved: {
            control.hue = value
            control.colorRequested(Qt.hsva(value, control.value.hsvSaturation, control.value.hsvValue, 1))
        }
    }
    Label { text: control.value.toString().toUpperCase(); color: "#CCCCCC" }
}

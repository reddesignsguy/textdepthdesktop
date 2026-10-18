import QtQuick
import QtTest
import "../.."

TestCase {
    name: "ColorPickerDrag"
    width: 300
    height: 300
    visible: running
    when: windowShown

    Flickable {
        id: scroll
        anchors.fill: parent
        contentHeight: 600
        clip: true

        RgbColorControl {
            id: picker
            y: 40
            width: 240
            value: "red"
            onColorRequested: function(color) { value = color }
        }
    }

    function test_fastDragKeepsUpdating() {
        const square = findChild(picker, "saturationBrightnessSquare")
        mousePress(square, 30, 140)
        try {
            // Cross the parent Flickable's drag threshold, then keep dragging.
            mouseMove(square, 50, 110, 1)
            mouseMove(square, 80, 70, 1)
            mouseMove(square, 140, 30, 1)
            verify(Math.abs(picker.value.hsvSaturation - 140 / square.width) < 0.01,
                   "The picker must keep receiving moves after a fast drag")
            compare(scroll.contentY, 0, "Dragging the picker must not scroll its parent")
            // Keep the grab outside the square and resume when returning inside.
            mouseMove(square, square.width + 20, 20, 1)
            verify(picker.value.hsvSaturation > 0.99)
            mouseMove(square, 60, 90, 1)
            verify(Math.abs(picker.value.hsvSaturation - 60 / square.width) < 0.01)
        } finally {
            mouseRelease(square, 60, 90)
        }
    }
}

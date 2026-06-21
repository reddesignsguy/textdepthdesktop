import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TextDepthOG

Window {
    width: 1920
    height: 1080
    visible: true
    title: qsTr("TextDepth Demo")
    color: "#212121"

    ColumnLayout {
        anchors.fill: parent

        // Input panel
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: topBar.implicitHeight
            anchors.margins: 10
            color: "#2B2B2B"

            ColumnLayout {
                anchors.fill: parent
                id: topBar
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 10

                Label {
                    text: "Enter text to render:"
                    font.pixelSize: 14
                    font.bold: true
                }
                Button {
                    id: tmp_Save
                    width: 50
                    font.pixelSize: 16
                    text: "Export to PSD"

                    onClicked: {
                         textDepthWidget.writeToPhotoshop();
                    }

                }
                TextField {
                    id: textInput
                    Layout.fillWidth: true
                    placeholderText: "Type your text here..."
                    text: "o"
                    font.pixelSize: 16
                    onTextChanged: {
                        textDepthWidget.text = text
                    }
                }

            }
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Rectangle {
                id: viewport

                Layout.fillWidth: true
                Layout.fillHeight: true

                color: "#212121"
                clip: true

                property real zoomFactor: 1.0

                //
                // Infinite scene
                //
                Item {
                id: scene

                // TODO:  Do we need this?
                //x: scenePosX
                //y: scenePosY

                //property real scenePosX:
                //    viewport.width / 2 - document.width / 2

                //property real scenePosY:
                //    viewport.height / 2 - document.height / 2

                scale: viewport.zoomFactor
                transformOrigin: Item.TopLeft

                Rectangle {
                    id: document
                    // Todo: allow user to choose canvas size
                    // https://github.com/reddesignsguy/textdepthdesktop/issues/3
                    width: 2000
                    height: 2000

                    color: "white"
                    border.color: "#808080"
                    border.width: 1

                    TextDepthViewport {
                    id: textDepthWidget

                    anchors.fill: parent
                    text: textInput.text
                    }
                }
                }

                //
                // Mouse drag pan
                //
                MouseArea {
                anchors.fill: parent

                acceptedButtons: Qt.MiddleButton | Qt.LeftButton

                property real lastX
                property real lastY

                onPressed: function(mouse) {
                    lastX = mouse.x
                    lastY = mouse.y
                }

                onPositionChanged: function(mouse) {
                    if (!pressed)
                    return

                    scene.x += mouse.x - lastX
                    scene.y += mouse.y - lastY

                    lastX = mouse.x
                    lastY = mouse.y
                }
                }

                //
                // Wheel zoom toward cursor
                //
                WheelHandler {
                target: null

                onWheel: function(event) {

                    var oldZoom = viewport.zoomFactor

                    var zoomStep =
                    event.angleDelta.y > 0
                    ? 1.1
                    : 0.9

                    var newZoom = oldZoom * zoomStep

                    newZoom = Math.max(0.05,
                           Math.min(20.0, newZoom))

                    var factor = newZoom / oldZoom

                    var mouseX = event.x
                    var mouseY = event.y

                    scene.x =
                    mouseX -
                    (mouseX - scene.x) * factor

                    scene.y =
                    mouseY -
                    (mouseY - scene.y) * factor

                    viewport.zoomFactor = newZoom
                }
                }

                //
                // Pinch zoom toward pinch center
                //
                PinchHandler {
                id: pinch
                target: null

                property real startZoom
                property real startSceneX
                property real startSceneY
                property real lastScale

                onActiveChanged: {
                    if (active) {
                    startZoom = viewport.zoomFactor
                    startSceneX = scene.x
                    startSceneY = scene.y
                    lastScale = scale
                    console.log("... staerting")
                    }
                }

                onScaleChanged: {
                    var zoomFactor = 0.025
                    var scaleDelta = scale - lastScale


                    var newZoom = viewport.zoomFactor
                    if (scaleDelta > 0) {
                    newZoom += zoomFactor
                    } else if (scaleDelta < 0) {
                    newZoom -= zoomFactor
                    }


                    newZoom = Math.max(0.05,
                               Math.min(20.0, newZoom))
                    var factor = newZoom / startZoom

                    var px = centroid.position.x
                    var py = centroid.position.y

                    scene.x =
                    px - (px - startSceneX) * factor

                    scene.y =
                    py - (py - startSceneY) * factor

                    viewport.zoomFactor = newZoom

                    lastScale = scale
                }
                }
            }
            Rectangle {
                id: root
                color: "#2B2B2B"
                Layout.preferredWidth: 300
                Layout.fillHeight: true
                property int selectedLayer: -1

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        height: 36
                        color: "#353535"

                        Text {
                            anchors.centerIn: parent
                            text: "Layers"
                            color: "white"
                            font.bold: true
                        }
                    }

                    ListView {
                        id: layerView

                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        model: layerModel
                        clip: true

                        delegate: Rectangle {

                            required property string layerName
                            required property bool layerVisible
                            required property int index

                            width: layerView.width
                            height: 40

                            color: root.selectedLayer === index
                                   ? "#4A6EA8"
                                   : "#3A3A3A"

                            border.color: "#555"
                            MouseArea {
                                anchors.fill: parent

                                onClicked: {
                                    root.selectedLayer = index
                                }
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8

                                CheckBox {
                                    checked: layerVisible

                                    onToggled: {
                                        layerModel.setData(
                                            layerModel.index(index, 0),
                                            checked,
                                            258 // VisibleRole
                                        )
                                    }
                                }

                                Rectangle {
                                    width: 28
                                    height: 28
                                    color: "#777"

                                    Text {
                                        anchors.centerIn: parent
                                        text: "🖼"
                                    }
                                }

                                Text {
                                    text: layerName
                                    color: "white"

                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                            }
                            Component.onCompleted: {
                                console.log("delegate created", layerName)
                                console.log("model =", layerModel)
                                console.log("rowCount =", layerModel.rowCount())

                            }
                        }
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        height: 40

                        color: "#353535"

                        RowLayout {
                            anchors.fill: parent

                            Button {
                                text: "+"

                                onClicked: {
                                    layerModel.addLayer("New Layer")
                                    console.log("detected click for adding new layer")
                                }
                            }

                            Button {
                                text: "-"

                                enabled: root.selectedLayer >= 0

                                onClicked: {
                                    layerModel.removeLayer(root.selectedLayer)
                                }
                            }
                        }
                    }
                }
            }
        }

    }
}

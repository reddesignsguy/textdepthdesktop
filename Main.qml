import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Window {
    id: mainWindow
    width: 1920
    height: 1080
    visible: true
    title: qsTr("TextDepth Demo")
    color: "#212121"

    Action {
        id: openPsdAction
        text: qsTr("Open PSD…")
        shortcut: StandardKey.Open
        onTriggered: psdFileDialog.open()
    }

    Action {
        id: exportPsdAction
        text: qsTr("Export to PSD")
        onTriggered: textDepthBackend.writeToPhotoshop()
    }

    FileDialog {
        id: psdFileDialog
        title: qsTr("Open PSD")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Photoshop documents (*.psd)")]
        onAccepted: textDepthBackend.loadPsd(selectedFile)
    }

    MessageDialog {
        id: loadErrorDialog
        title: qsTr("Unable to open PSD")
        buttons: MessageDialog.Ok
    }

    Connections {
        target: textDepthBackend
        function onUnitEditFailed(message) {
            loadErrorDialog.title = qsTr("Unable to update unit")
            loadErrorDialog.text = message
            loadErrorDialog.open()
        }
        function onPsdLoadFailed(message) {
            loadErrorDialog.title = qsTr("Unable to open PSD")
            loadErrorDialog.text = message
            loadErrorDialog.open()
        }
    }

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
                ActionToolBar {
                    Layout.fillWidth: true
                    actions: [openPsdAction, exportPsdAction]
                }
                CanvasToolBar {
                    id: canvasTools
                    Layout.fillWidth: true
                }
                TextField {
                    id: textInput
                    Layout.fillWidth: true
                    placeholderText: "Type your text here..."
                    text: "o"
                    font.pixelSize: 16
                    onTextChanged: {
                        textDepthBackend.text = text
                    }
                }

            }
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Rectangle {
                id: viewport
                objectName: "documentViewport"

                Layout.fillWidth: true
                Layout.fillHeight: true

                color: "#212121"
                clip: true


                Item {
                    id: canvasScene
                    scale: 1.0
                    transformOrigin: Item.TopLeft

                    Rectangle {
                        id: document
                        // TODO: Allow choosing the canvas size.
                        width: 1920
                        height: 1080
                        color: "white"
                        border.color: "#808080"
                        border.width: 1

                        Item {
                            id: textDepthViewportHost
                            anchors.fill: parent
                            Component.onCompleted: {
                                textDepthWidget.parent = textDepthViewportHost
                                textDepthWidget.width = width
                                textDepthWidget.height = height
                            }
                            onWidthChanged: textDepthWidget.width = width
                            onHeightChanged: textDepthWidget.height = height
                        }
                    }
                }

                NavigationTool {
                    anchors.fill: parent
                    scene: canvasScene
                    enabled: canvasTools.activeTool === "navigate"
                    visible: enabled
                }
                BackMoveTool {
                    anchors.fill: parent
                    scene: canvasScene
                    propertiesModel: unitPropertiesModel
                    enabled: canvasTools.activeTool === "backMove"
                    visible: enabled
                }
            }
            ColumnLayout {
                objectName: "editorSidebar"
                // Layouts expand by default. Bound the sidebar so the viewport
                // receives the remaining width, including in smaller windows.
                Layout.fillWidth: false
                Layout.preferredWidth: Math.min(300, mainWindow.width * 0.4)
                Layout.minimumWidth: Layout.preferredWidth
                Layout.maximumWidth: Layout.preferredWidth
                Layout.fillHeight: true
                spacing: 8
                LayersPanel {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    layerModel: documentLayerModel
                }
                UnitPropertiesPanel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.min(implicitHeight, mainWindow.height * 0.55)
                    Layout.minimumHeight: 0
                    propertiesModel: unitPropertiesModel
                }
            }
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: panel
    required property var backend
    required property var propertiesModel
    objectName: "globalOptionsPanel"
    color: "#2B2B2B"
    implicitHeight: content.implicitHeight + 24

    ColumnLayout {
        id: content
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8
        Label {
            text: qsTr("Global options")
            color: "white"
            font.bold: true
        }
        CheckBox {
            objectName: "synchronizeLayersCheckBox"
            Layout.fillWidth: true
            text: qsTr("Synchronize layers")
            checked: panel.backend.synchronizeLayers
            onToggled: {
                panel.propertiesModel.cancelTranslation()
                panel.propertiesModel.cancelResize()
                panel.backend.synchronizeLayers = checked
            }
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Move and scale all layers together around a shared center, including hidden layers.")
        }
    }
}

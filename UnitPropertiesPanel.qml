import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: panel
    required property var propertiesModel
    color: "#2B2B2B"
    implicitHeight: content.implicitHeight + 24

    ScrollView {
        id: scroll
        anchors.fill: parent
        anchors.margins: 12
        contentWidth: availableWidth
        clip: true
        ColumnLayout {
            id: content
            width: scroll.availableWidth
            spacing: 12

            Label {
                text: qsTr("Unit properties")
                color: "white"
                font.bold: true
            }
            Label {
                Layout.fillWidth: true
                visible: !panel.propertiesModel.hasSelection
                text: qsTr("Select a layer to customize it")
                color: "#AAAAAA"
                wrapMode: Text.WordWrap
            }
            ColumnLayout {
                Layout.fillWidth: true
                visible: panel.propertiesModel.hasSelection
                Label {
                    text: qsTr("Back")
                    font.bold: true
                    color: "white"
                }
                Button {
                    objectName: "backToggle"
                    Layout.fillWidth: true
                    text: panel.propertiesModel.backVisible ? qsTr("Hide back") : qsTr("Show back")
                    enabled: panel.propertiesModel.hasSelection
                             && (panel.propertiesModel.canShowBack || panel.propertiesModel.backVisible)
                    // State always comes from a confirmed backend update.
                    highlighted: panel.propertiesModel.backVisible
                    onClicked: panel.propertiesModel.requestToggleBack()
                }
                Label {
                    Layout.fillWidth: true
                    visible: !panel.propertiesModel.canShowBack && !panel.propertiesModel.backVisible
                    text: qsTr("A front vector mask is required for a back.")
                    color: "#AAAAAA"
                    wrapMode: Text.WordWrap
                }
                BackColorsPanel {
                    Layout.fillWidth: true
                    propertiesModel: panel.propertiesModel
                }
            }
        }
    }
}

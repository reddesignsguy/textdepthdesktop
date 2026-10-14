import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ToolBar {
    id: toolbar
    property string activeTool: "navigate"
    readonly property var tools: [
        { key: "navigate", label: qsTr("Navigate canvas"), icon: "icons/navigate.svg" },
        { key: "backMove", label: qsTr("Move back"), icon: "icons/back-move.svg" }
    ]
    ButtonGroup { id: toolGroup }
    RowLayout {
        Repeater {
            model: toolbar.tools
            ToolButton {
                required property var modelData
                objectName: modelData.key + "ToolButton"
                icon.source: modelData.icon
                text: modelData.label
                display: AbstractButton.IconOnly
                checkable: true
                checked: toolbar.activeTool === modelData.key
                ButtonGroup.group: toolGroup
                onClicked: toolbar.activeTool = modelData.key
                ToolTip.visible: hovered
                ToolTip.text: text
                Accessible.name: text
            }
        }
    }
}

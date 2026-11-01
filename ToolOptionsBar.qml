import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ToolBar {
    id: toolbar
    property string activeTool: "navigate"
    property bool moveVerticalOnly: false
    visible: activeTool === "backMove"

    RowLayout {
        CheckBox {
            objectName: "moveVerticalOnlyCheckBox"
            text: qsTr("Vertical only")
            checked: toolbar.moveVerticalOnly
            onToggled: toolbar.moveVerticalOnly = checked
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Restrict movement to the vertical axis")
        }
    }
}

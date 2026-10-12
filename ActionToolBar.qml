import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Add commands through actions; buttons share their enabled state and labels.
ToolBar {
    id: toolbar
    property list<Action> actions

    contentItem: RowLayout {
        Repeater {
            model: toolbar.actions
            delegate: ToolButton {
                required property var modelData
                action: modelData
            }
        }
        Item { Layout.fillWidth: true }
    }
}

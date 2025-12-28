import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TextDepthOG

Window {
    width: 640
    height: 580
    visible: true
    title: qsTr("TextDepth Demo")
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20
        
        // Input panel
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            color: "#f5f5f5"
            border.color: "#cccccc"
            border.width: 1
            radius: 5
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 10
                
                Label {
                    text: "Enter text to render:"
                    font.pixelSize: 14
                    font.bold: true
                }
                
                TextField {
                    id: textInput
                    Layout.fillWidth: true
                    placeholderText: "Type your text here..."
                    text: "TextDepth"
                    font.pixelSize: 16
                    
                    onTextChanged: {
                        textDepthWidget.text = text
                    }
                }
            }
        }
        
        // TextDepth widget
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "white"
            border.color: "#cccccc"
            border.width: 1
            radius: 5
            
            TextDepth {
                id: textDepthWidget
                anchors.centerIn: parent
                width: parent.width - 40
                height: parent.height - 40
                text: textInput.text
            }
        }
    }
}

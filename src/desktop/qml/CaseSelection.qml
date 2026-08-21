import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: caseSelectionPage
    
    Rectangle {
        anchors.centerIn: parent
        width: 400
        height: 350
        color: "#2a2a2a"
        radius: 10
        border.color: "#3a3a3a"
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 30
            spacing: 20
            
            Text {
                text: "ForensicVM"
                color: "white"
                font.pixelSize: 24
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }
            
            Text {
                text: "Select a Case"
                color: "#aaaaaa"
                font.pixelSize: 16
                Layout.alignment: Qt.AlignHCenter
            }
            
            TextField {
                id: pathField
                Layout.fillWidth: true
                placeholderText: "Case Directory Path"
                text: ""
                color: "white"
                background: Rectangle {
                    color: "#1e1e1e"
                    border.color: "#444"
                    radius: 4
                }
            }

            TextField {
                id: nameField
                Layout.fillWidth: true
                placeholderText: "Case Name (For Creation)"
                text: ""
                color: "white"
                background: Rectangle {
                    color: "#1e1e1e"
                    border.color: "#444"
                    radius: 4
                }
            }

            TextField {
                id: invField
                Layout.fillWidth: true
                placeholderText: "Investigator (For Creation)"
                text: ""
                color: "white"
                background: Rectangle {
                    color: "#1e1e1e"
                    border.color: "#444"
                    radius: 4
                }
            }
            
            RowLayout {
                Layout.fillWidth: true
                spacing: 15
                
                Button {
                    text: "Create Case"
                    Layout.fillWidth: true
                    onClicked: {
                        appViewModel.createCase(pathField.text, nameField.text, invField.text)
                    }
                }
                
                Button {
                    text: "Open Case"
                    Layout.fillWidth: true
                    onClicked: {
                        appViewModel.openCase(pathField.text)
                    }
                }
            }
        }
    }
}

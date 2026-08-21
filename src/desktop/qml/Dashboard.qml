import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: dashboardPage
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20
        
        RowLayout {
            Layout.fillWidth: true
            
            Text {
                text: "Dashboard - " + dashboardViewModel.caseName
                color: "white"
                font.pixelSize: 28
                font.bold: true
                Layout.fillWidth: true
            }
            
            Button {
                text: "Close Case"
                onClicked: appViewModel.closeCase()
            }
        }
        
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 20
            
            // Left Column (VMs / Operations)
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 60
                
                Text {
                    text: "Virtual Machines"
                    color: "white"
                    font.pixelSize: 18
                }
                
                // Example hardcoded VM for Phase 6
                Rectangle {
                    Layout.fillWidth: true
                    height: 120
                    color: "#2a2a2a"
                    border.color: "#444"
                    radius: 5
                    
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        
                        Text {
                            text: "VM-1 (Win10-Test)"
                            color: "white"
                            font.bold: true
                        }
                        
                        RowLayout {
                            Button { text: "Start"; onClicked: dashboardViewModel.launchSession("00000000-0000-0000-0000-000000000001") }
                            Button { text: "Stop"; onClicked: dashboardViewModel.stopSession("00000000-0000-0000-0000-000000000001") }
                        }
                        RowLayout {
                            Button { text: "Acquire Memory"; onClicked: dashboardViewModel.acquireMemory("00000000-0000-0000-0000-000000000001") }
                            Button { text: "Acquire Disk Delta"; onClicked: dashboardViewModel.acquireDiskDelta("00000000-0000-0000-0000-000000000001") }
                        }
                    }
                }
                
                Item { Layout.fillHeight: true } // Spacer
            }
            
            // Right Column (Evidence / Status)
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 40
                
                Text {
                    text: "Active Operations"
                    color: "white"
                    font.pixelSize: 18
                }
                
                ListView {
                    id: opsList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: operationManager.activeOperations
                    
                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 60
                        color: "#2a2a2a"
                        border.color: "#444"
                        radius: 4
                        margin: 5
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            
                            ColumnLayout {
                                Layout.fillWidth: true
                                Text { text: modelData.message; color: "white"; font.bold: true; elide: Text.ElideRight; Layout.fillWidth: true }
                                Text { text: "State: " + modelData.state; color: "#aaa" }
                            }
                            
                            Button {
                                text: "Cancel"
                                onClicked: operationManager.cancelOperation(modelData.id)
                                visible: modelData.state === 1 || modelData.state === 2 // InProgress or Queued
                            }
                        }
                    }
                }
            }
        }
    }
}

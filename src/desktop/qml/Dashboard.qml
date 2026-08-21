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
                RowLayout {
                    Layout.fillWidth: true
                    
                    Text {
                        text: "Virtual Machines"
                        color: "white"
                        font.pixelSize: 18
                        Layout.fillWidth: true
                    }
                    
                    Button {
                        text: "New Session"
                        onClicked: {
                            // Generate a simple unique ID
                            function s4() { return Math.floor((1 + Math.random()) * 0x10000).toString(16).substring(1); }
                            var uuid = s4() + s4() + '-' + s4() + '-' + s4() + '-' + s4() + '-' + s4() + s4() + s4();
                            dashboardViewModel.launchSession(uuid)
                        }
                    }
                }
                
                ListView {
                    id: vmListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: dashboardViewModel.vmList
                    
                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 120
                        color: "#2a2a2a"
                        border.color: "#444"
                        radius: 5
                        margin: 5
                        
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            
                            Text {
                                text: "VM: " + modelData.id
                                color: "white"
                                font.bold: true
                            }
                            
                            RowLayout {
                                Button { text: "Start"; onClicked: dashboardViewModel.launchSession(modelData.id) }
                                Button { text: "Stop"; onClicked: dashboardViewModel.stopSession(modelData.id) }
                            }
                            RowLayout {
                                Button { text: "Acquire Memory"; onClicked: dashboardViewModel.acquireMemory(modelData.id) }
                                Button { text: "Acquire Disk Delta"; onClicked: dashboardViewModel.acquireDiskDelta(modelData.id) }
                            }
                        }
                    }
                }
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

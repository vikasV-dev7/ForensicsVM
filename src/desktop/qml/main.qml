import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: mainWindow
    width: 1024
    height: 768
    visible: true
    title: "ForensicVM"

    color: "#1e1e1e"

    // Material theme settings (if QtQuick.Controls.Material is used)
    // Material.theme: Material.Dark
    // Material.accent: Material.Blue

    Loader {
        id: pageLoader
        anchors.fill: parent
        // Bind source to application state
        source: appViewModel.isCaseOpen ? "qrc:/qml/Dashboard.qml" : "qrc:/qml/CaseSelection.qml"
    }

    // Global Error overlay
    Rectangle {
        id: errorOverlay
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 20
        width: Math.min(parent.width - 40, 600)
        height: errorText.contentHeight + 40
        color: "#c62828"
        radius: 8
        visible: appViewModel.errorMessage !== ""
        
        Text {
            id: errorText
            anchors.centerIn: parent
            width: parent.width - 40
            color: "white"
            text: appViewModel.errorMessage
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 14
        }
        
        Timer {
            id: errorTimer
            interval: 5000 // Hide after 5 seconds
            running: appViewModel.errorMessage !== ""
            onTriggered: {
                // We'd need a clearError function or we can just hide it locally
                errorOverlay.visible = false
            }
        }
    }
}

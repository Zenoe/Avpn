import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2
import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Components"

PageType {
    id: root

    property var processedServer

    SortFilterProxyModel {
        id: processedServerModel
        sourceModel: ServersModel
        filters: ValueFilter {
            roleName: "serverId"
            value: ServersUiController.processedServerId
        }
    }

    function updateProcessedServer() {
        processedServer = processedServerModel.count > 0 ? processedServerModel.get(0) : null
    }

    Connections {
        target: ServersModel
        function onModelReset() { root.updateProcessedServer() }
    }

    Connections {
        target: ServersUiController
        function onProcessedServerIdChanged() { root.updateProcessedServer() }
    }

    Component.onCompleted: updateProcessedServer()

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 20 + PageController.safeAreaTopMargin
        spacing: 4

        BackButtonType {}

        HeaderTypeWithButton {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 10
            actionButtonImage: "qrc:/images/controls/edit-3.svg"
            headerText: root.processedServer ? root.processedServer.name : ""
            descriptionText: root.processedServer ? root.processedServer.hostName : ""
            actionButtonFunction: function() { renameDrawer.openTriggered() }
        }

        RenameServerDrawer {
            id: renameDrawer
            parent: root
            anchors.fill: parent
            expandedHeight: root.height * 0.35
            serverNameText: root.processedServer ? root.processedServer.name : ""
        }

        TabBar {
            id: tabBar
            Layout.fillWidth: true
            background: Rectangle { color: CaelispectStyle.color.transparent }

            TabButtonType {
                text: qsTr("Protocols")
                isSelected: tabBar.currentIndex === 0
            }
            TabButtonType {
                text: qsTr("Management")
                isSelected: tabBar.currentIndex === 1
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            PageSettingsServerProtocols { stackView: root.stackView }
            PageSettingsServerData { stackView: root.stackView }
        }
    }
}

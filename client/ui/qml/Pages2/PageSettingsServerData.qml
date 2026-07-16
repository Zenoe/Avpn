import QtQuick
import QtQuick.Layouts
import Style 1.0

import "./"
import "../Controls2"

PageType {
    id: root

    ListViewType {
        anchors.fill: parent

        delegate: ColumnLayout {
            width: ListView.view.width

            LabelWithButtonType {
                Layout.fillWidth: true
                text: qsTr("Remove server from application")
                textColor: AmneziaStyle.color.vibrantRed
                clickedFunction: function() {
                    showQuestionDrawer(
                        qsTr("Do you want to remove the server from application?"),
                        "",
                        qsTr("Continue"),
                        qsTr("Cancel"),
                        function() {
                            if (ServersUiController.isDefaultServerCurrentlyProcessed()
                                    && ConnectionController.isConnected) {
                                PageController.showNotificationMessage(
                                    qsTr("Cannot remove server during active connection"))
                                return
                            }
                            ServersUiController.removeServer(ServersUiController.processedServerId)
                            PageController.closePage()
                        },
                        function() {})
                }
            }

            DividerType {}
        }

        model: 1
    }
}

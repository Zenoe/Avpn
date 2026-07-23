import QtQuick
import QtQuick.Layouts
import PageEnum 1.0

import "./"
import "../Controls2"

PageType {
    ListViewType {
        anchors.fill: parent
        model: 1
        delegate: ColumnLayout {
            width: ListView.view.width
            LabelWithButtonType {
                Layout.fillWidth: true
                text: "WireGuard"
                descriptionText: qsTr("Standard WireGuard profile")
                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                clickedFunction: function() {
                    ServersUiController.openClientProtocolSettings(ServersUiController.processedServerId)
                    PageController.goToPage(PageEnum.PageProtocolWireGuardClientSettings)
                }
            }
            DividerType { Layout.fillWidth: true }
        }
    }
}

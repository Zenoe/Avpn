import QtQuick
import QtQuick.Layouts
import SortFilterProxyModel 0.2
import ContainersModelFilters 1.0
import PageEnum 1.0

import "./"
import "../Controls2"

PageType {
    id: root

    ListViewType {
        id: protocolsList
        anchors.fill: parent

        model: SortFilterProxyModel {
            id: protocolsModel
            sourceModel: ContainersModel
            filters: ContainersModelFilters.getReadAccessProtocolsListFilters()
        }

        delegate: ColumnLayout {
            width: protocolsList.width

            LabelWithButtonType {
                readonly property bool isAwgProtocol: dockerContainer === 1 || dockerContainer === 2
                readonly property bool isWireGuardProtocol: dockerContainer === 3
                Layout.fillWidth: true
                text: name
                descriptionText: description
                rightImageSource: (isAwgProtocol || isWireGuardProtocol) ? "qrc:/images/controls/chevron-right.svg" : ""

                clickedFunction: function() {
                    if (!isAwgProtocol && !isWireGuardProtocol) return
                    var containerIndex = protocolsModel.mapToSource(index)
                    ServersUiController.processedContainerIndex = containerIndex
                    ServersUiController.openClientProtocolSettings(
                                ServersUiController.processedServerId,
                                containerIndex,
                                isAwgProtocol ? 3 : 2)
                    PageController.goToPage(isAwgProtocol
                                            ? PageEnum.PageProtocolAwgClientSettings
                                            : PageEnum.PageProtocolWireGuardClientSettings)
                }
            }

            DividerType {}
        }
    }
}

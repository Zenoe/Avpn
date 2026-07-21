import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"

Item {
    id: root

    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        color: CaelispectStyle.color.midnightBlack
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 32, 480)
        spacing: 20

        Header1TextType {
            Layout.fillWidth: true
            text: qsTr("零信任访问")
            horizontalAlignment: Text.AlignHCenter
        }

        TextFieldWithHeaderType {
            id: gatewayInput

            Layout.fillWidth: true
            headerText: qsTr("请输入零信任地址")
            enabled: !SpaController.busy

            Component.onCompleted: {
                textField.text = "192.168.1.122"
                textField.placeholderText = qsTr("例如：spa.example.com")
                textField.forceActiveFocus()
            }

            textField.onAccepted: {
                if (confirmButton.enabled) {
                    SpaController.start(textField.text)
                }
            }
        }

        BasicButtonType {
            id: confirmButton

            Layout.fillWidth: true
            text: SpaController.busy ? qsTr("请求中…") : qsTr("确定")
            enabled: !SpaController.busy && gatewayInput.textField.text.trim().length > 0
            clickedFunc: function() {
                SpaController.start(gatewayInput.textField.text)
            }
        }

        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            running: SpaController.busy
            visible: running
        }

        LabelTextType {
            Layout.fillWidth: true
            visible: SpaController.statusMessage.length > 0
            text: SpaController.statusMessage
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            color: {
                if (SpaController.statusType === "success") {
                    return CaelispectStyle.color.goldenApricot
                }
                if (SpaController.statusType === "error") {
                    return CaelispectStyle.color.vibrantRed
                }
                return CaelispectStyle.color.mutedGray
            }
        }
    }
}

import QtQuick
import QtQuick.Controls

Item {
    id: root

    anchors.fill: parent

    property bool compactMode: width < 980

    readonly property color primaryBlue: "#137fd9"
    readonly property color primaryText: "#30323a"
    readonly property color secondaryText: "#8c919d"
    readonly property color successGreen: "#15AA12"
    readonly property color warningYellow: "#FFC949"
    readonly property color errorRed: "#FF362A"

    Rectangle {
        anchors.fill: parent
        color: "#f5f7fb"
    }

    Row {
        anchors.fill: parent

        Item {
            id: introductionPanel

            width: root.compactMode ? 0 : parent.width * 0.42
            height: parent.height
            visible: width > 0

            Rectangle {
                anchors.fill: parent
                color: "#f4f7fc"
            }

            Column {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: -36
                width: Math.min(parent.width - 88, 480)
                spacing: 20

                Row {
                    spacing: 22

                    Image {
                        source: "qrc:/images/spa/暂时图标.png"
                        width: 73
                        height: 73
                        fillMode: Image.PreserveAspectFit
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 12

                        Text {
                            text: qsTr("天鉴零信任")
                            color: root.primaryText
                            font.pixelSize: 30
                            font.weight: Font.DemiBold
                        }

                        Text {
                            text: qsTr("构建端到端的安全接入访问关系")
                            color: root.secondaryText
                            font.pixelSize: 14
                        }
                    }
                }

                Item { width: 1; height: 10 }

                CapabilityCard {
                    width: parent.width
                    iconSource: "qrc:/images/spa/Group_367.png"
                    title: qsTr("零信任安全架构")
                    description: qsTr("基于 SDP 理念，持续验证，永不信任")
                }

                CapabilityCard {
                    width: parent.width
                    iconSource: "qrc:/images/spa/认证.png"
                    title: qsTr("多因子身份认证")
                    description: qsTr("支持生物识别、动态口令等多种认证方式")
                }

                CapabilityCard {
                    width: parent.width
                    iconSource: "qrc:/images/spa/链路.png"
                    title: qsTr("全链路流量加密")
                    description: qsTr("国密算法加密传输，数据安全有保障")
                }
            }

        }

        Item {
            id: connectPanel

            width: parent.width - introductionPanel.width
            height: parent.height

            Image {
                anchors.fill: parent
                source: "qrc:/images/spa/Mask_group.png"
                // The source artwork includes the left-side blank panel. Only use
                // its right-side network illustration inside the connection panel.
                sourceClipRect: Qt.rect(603, 0, 837, 1091)
                fillMode: Image.PreserveAspectCrop
                horizontalAlignment: Image.Center
                verticalAlignment: Image.Center
            }

            Rectangle {
                id: connectCard

                anchors.centerIn: parent
                width: Math.min(parent.width - 48, 540)
                height: 392
                radius: 24
                color: "white"

                Column {
                    anchors.fill: parent
                    anchors.margins: 48
                    spacing: 28

                    Text {
                        text: qsTr("请输入零信任地址")
                        color: root.primaryText
                        font.pixelSize: 30
                        font.weight: Font.Normal
                    }

                    Item {
                        width: parent.width
                        height: 92

                        Rectangle {
                            id: inputFrame

                            width: parent.width
                            height: 58
                            radius: 10
                            color: "white"
                            border.width: 1
                            border.color: gatewayInput.activeFocus ? root.primaryBlue : "#e1e4e9"

                            Image {
                                anchors.left: parent.left
                                anchors.leftMargin: 19
                                anchors.verticalCenter: parent.verticalCenter
                                source: "qrc:/images/spa/地址_(1) 1.png"
                                width: 18
                                height: 18
                                fillMode: Image.PreserveAspectFit
                            }

                            TextField {
                                id: gatewayInput

                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.leftMargin: 50
                                anchors.rightMargin: 16
                                enabled: !SpaController.busy
                                color: root.primaryText
                                text: "192.168.1.122"
                                placeholderText: qsTr("请输入地址")
                                placeholderTextColor: "#a5a9b2"
                                font.pixelSize: 15
                                selectByMouse: true
                                topPadding: 0
                                bottomPadding: 0
                                leftPadding: 0
                                rightPadding: 0

                                background: Item {}

                                Component.onCompleted: forceActiveFocus()

                                onAccepted: {
                                    if (connectButton.enabled) {
                                        SpaController.start(text)
                                    }
                                }
                            }
                        }

                        Row {
                            anchors.top: inputFrame.bottom
                            anchors.topMargin: 8
                            width: parent.width
                            height: 26
                            spacing: 8

                            BusyIndicator {
                                anchors.verticalCenter: parent.verticalCenter
                                running: SpaController.busy
                                visible: running
                                width: 18
                                height: 18
                            }

                            Text {
                                id: statusText

                                width: parent.width - (SpaController.busy ? 26 : 0)
                                anchors.verticalCenter: parent.verticalCenter
                                visible: SpaController.statusMessage.length > 0
                                text: SpaController.statusMessage
                                elide: Text.ElideRight
                                color: {
                                    if (SpaController.statusType === "success") {
                                        return root.successGreen
                                    }
                                    if (SpaController.statusType === "error") {
                                        return root.errorRed
                                    }
                                    return root.warningYellow
                                }
                                font.pixelSize: 13
                            }
                        }
                    }

                    Button {
                        id: connectButton

                        width: parent.width
                        height: 56
                        enabled: !SpaController.busy && gatewayInput.text.trim().length > 0
                        text: SpaController.busy ? qsTr("连接中…") : qsTr("连接")
                        hoverEnabled: true

                        onClicked: SpaController.start(gatewayInput.text)

                        contentItem: Text {
                            text: connectButton.text
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 19
                            font.weight: Font.Medium
                        }

                        background: Rectangle {
                            radius: 8
                            color: {
                                if (!connectButton.enabled) {
                                    return "#9dcaee"
                                }
                                if (SpaController.succeeded) {
                                    return connectButton.down ? "#0f8a0d"
                                                               : (connectButton.hovered ? "#19bb16" : root.successGreen)
                                }
                                if (connectButton.down) {
                                    return "#0d6fbe"
                                }
                                return connectButton.hovered ? "#1888e6" : root.primaryBlue
                            }
                        }
                    }

                }
            }
        }
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 28
        width: parent.width
        text: qsTr("© 2024 天鉴零信任安全平台\n安全运营中心 | 技术支持热线：400-xxx-xxxx")
        color: "#858a94"
        horizontalAlignment: Text.AlignHCenter
        lineHeight: 1.35
        font.pixelSize: 13
    }

    component CapabilityCard: Rectangle {
        id: capabilityCard

        property string iconSource
        property string title
        property string description

        height: 84
        radius: 16
        color: "#ffffff"

        Row {
            anchors.fill: parent
            anchors.leftMargin: 24
            anchors.rightMargin: 20
            spacing: 16

            Image {
                anchors.verticalCenter: parent.verticalCenter
                source: capabilityCard.iconSource
                width: 46
                height: 46
                fillMode: Image.PreserveAspectFit
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - 86
                spacing: 7

                Text {
                    text: capabilityCard.title
                    color: "#464953"
                    font.pixelSize: 15
                    font.weight: Font.Medium
                }

                Text {
                    width: parent.width
                    text: capabilityCard.description
                    color: "#9398a2"
                    elide: Text.ElideRight
                    font.pixelSize: 13
                }
            }
        }
    }
}

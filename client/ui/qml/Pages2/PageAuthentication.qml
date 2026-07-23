import QtQuick
import QtQuick.Controls

Item {
    id: root

    anchors.fill: parent

    readonly property color primaryBlue: "#137fd9"
    readonly property color primaryText: "#30323a"
    readonly property color secondaryText: "#8c919d"
    readonly property color successGreen: "#15AA12"
    readonly property color warningYellow: "#FFC949"
    readonly property color errorRed: "#FF362A"

    property var localProvider: providerForType("LOCAL")
    property var phoneProvider: providerForType("PHONE")
    property var qrProvider: providerForQr()
    property var externalProviders: externalProviderList()
    property string selectedMode: ""

    function providerForType(type) {
        for (let index = 0; index < AuthController.providers.length; ++index) {
            if (AuthController.providers[index].type === type) {
                return AuthController.providers[index]
            }
        }
        return ({})
    }

    function providerForQr() {
        for (let index = 0; index < AuthController.providers.length; ++index) {
            const type = AuthController.providers[index].type
            if (type === "DINGTALK" || type === "WECHAT" || type === "QR") {
                return AuthController.providers[index]
            }
        }
        return ({})
    }

    function externalProviderList() {
        const result = []
        for (let index = 0; index < AuthController.providers.length; ++index) {
            const provider = AuthController.providers[index]
            if (provider.type !== "LOCAL" && provider.type !== "PHONE") {
                result.push(provider)
            }
        }
        return result
    }

    function selectAvailableMode() {
        if (localProvider.id > 0) {
            selectedMode = "local"
            return
        }
        if (phoneProvider.id > 0) {
            selectedMode = "phone"
            return
        }
        if (qrProvider.id > 0) {
            selectedMode = "qr"
            return
        }
        selectedMode = ""
    }

    Component.onCompleted: {
        externalProviders = externalProviderList()
        selectAvailableMode()
    }

    Connections {
        target: AuthController

        function onProvidersChanged() {
            root.localProvider = root.providerForType("LOCAL")
            root.phoneProvider = root.providerForType("PHONE")
            root.qrProvider = root.providerForQr()
            root.externalProviders = root.externalProviderList()
            root.selectAvailableMode()
        }
    }

    Image {
        anchors.fill: parent
        source: "qrc:/images/login/Mask_group.png"
        fillMode: Image.Stretch
    }

    Item {
        id: brandPanel

        width: parent.width * 0.5
        height: parent.height
        visible: parent.width >= 980

        Column {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: -34
            spacing: 30

            Row {
                spacing: 28

                Image {
                    source: "qrc:/images/login/icon.png"
                    width: 88
                    height: 100
                    fillMode: Image.PreserveAspectFit
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 15

                    Text {
                        text: qsTr("天鉴零信任")
                        color: root.primaryText
                        font.pixelSize: 34
                        font.weight: Font.DemiBold
                    }

                    Text {
                        text: qsTr("构建端到端的安全接入访问关系")
                        color: root.secondaryText
                        font.pixelSize: 15
                    }
                }
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 16

                BrandBadge {
                    iconSource: "qrc:/images/login/架构.png"
                    text: qsTr("零信任安全架构")
                }
                BrandBadge {
                    iconSource: "qrc:/images/login/认证.png"
                    text: qsTr("多因子身份认证")
                }
                BrandBadge {
                    iconSource: "qrc:/images/login/链路.png"
                    text: qsTr("全链路流量加密")
                }
            }
        }
    }

    Rectangle {
        id: loginCard

        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: parent.width >= 980 ? parent.width * 0.07 : Math.max(24, parent.width * 0.05)
        width: Math.min(parent.width - 48, 540)
        height: 750 + Math.max(0, Math.ceil(root.externalProviders.length / 2) - 1) * 70
        radius: 28
        color: "white"

        Column {
            anchors.fill: parent
            anchors.margins: 46
            spacing: 20

            Text {
                text: qsTr("欢迎登录")
                color: root.primaryText
                font.pixelSize: 32
                font.weight: Font.Normal
            }

            Text {
                text: qsTr("请使用企业账号登录零信任安全门户")
                color: "#5e626d"
                font.pixelSize: 14
            }

            Row {
                width: parent.width
                height: 36
                spacing: 28

                LoginTab {
                    visible: root.localProvider.id > 0
                    text: qsTr("账号登录")
                    active: root.selectedMode === "local"
                    onClicked: {
                        root.selectedMode = "local"
                        AuthController.loadCaptcha(root.localProvider.id)
                    }
                }

                LoginTab {
                    visible: root.phoneProvider.id > 0
                    text: qsTr("手机验证")
                    active: root.selectedMode === "phone"
                    onClicked: root.selectedMode = "phone"
                }

                LoginTab {
                    visible: root.qrProvider.id > 0
                    text: qsTr("扫码登录")
                    active: root.selectedMode === "qr"
                    onClicked: root.selectedMode = "qr"
                }
            }

            Item {
                width: parent.width
                height: 300

                Column {
                    anchors.fill: parent
                    visible: root.selectedMode === "local"
                    spacing: 14

                    LoginInput {
                        id: usernameInput
                        width: parent.width
                        iconSource: "qrc:/images/login/人员.png"
                        placeholder: qsTr("请输入工号/用户名")
                        enabled: !AuthController.busy
                    }

                    LoginInput {
                        id: passwordInput
                        width: parent.width
                        iconSource: "qrc:/images/login/密码.png"
                        placeholder: qsTr("请输入密码")
                        echoMode: TextInput.Password
                        enabled: !AuthController.busy
                    }

                    Row {
                        width: parent.width
                        height: 58
                        spacing: 14
                        visible: AuthController.captchaRequired

                        LoginInput {
                            id: captchaInput
                            width: parent.width - 164
                            height: parent.height
                            iconSource: "qrc:/images/login/验证码.png"
                            placeholder: qsTr("请输入验证码")
                            enabled: !AuthController.busy
                        }

                        Rectangle {
                            width: 150
                            height: parent.height
                            radius: 8
                            color: "#edf5fe"
                            border.color: "#e1e4e9"

                            Image {
                                anchors.fill: parent
                                anchors.margins: 4
                                source: AuthController.captchaImageDataUrl
                                fillMode: Image.PreserveAspectFit
                            }

                            MouseArea {
                                anchors.fill: parent
                                enabled: !AuthController.busy
                                cursorShape: Qt.PointingHandCursor
                                onClicked: AuthController.loadCaptcha(root.localProvider.id)
                            }
                        }
                    }

                    Row {
                        width: parent.width
                        visible: AuthController.captchaRequired

                        Text {
                            text: qsTr("点击验证码图片刷新")
                            color: root.secondaryText
                            font.pixelSize: 12
                        }
                    }

                    Button {
                        id: localLoginButton

                        width: parent.width
                        height: 56
                        enabled: !AuthController.busy
                                 && usernameInput.value.trim().length > 0
                                 && passwordInput.value.length > 0
                                 && (!AuthController.captchaRequired || captchaInput.value.trim().length > 0)
                        text: AuthController.busy ? qsTr("登录中…") : qsTr("登录")
                        hoverEnabled: true
                        onClicked: AuthController.login(usernameInput.value, passwordInput.value,
                                                        captchaInput.value, root.localProvider.id)

                        contentItem: Text {
                            text: localLoginButton.text
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 20
                            font.weight: Font.Medium
                        }

                        background: Rectangle {
                            radius: 8
                            color: {
                                if (!localLoginButton.enabled) {
                                    return "#9dcaee"
                                }
                                if (AuthController.authenticated) {
                                    return root.successGreen
                                }
                                return localLoginButton.down ? "#0d6fbe"
                                                             : (localLoginButton.hovered ? "#1888e6" : root.primaryBlue)
                            }
                        }
                    }
                }

                Column {
                    anchors.fill: parent
                    visible: root.selectedMode === "phone"
                    spacing: 18

                    LoginInput {
                        id: phoneInput
                        width: parent.width
                        iconSource: "qrc:/images/login/手机.png"
                        placeholder: qsTr("请输入手机号码")
                    }

                    Row {
                        width: parent.width
                        height: 58
                        spacing: 14

                        LoginInput {
                            id: phoneCodeInput
                            width: parent.width - 164
                            height: parent.height
                            iconSource: "qrc:/images/login/验证码.png"
                            placeholder: qsTr("请输入验证码")
                        }

                        Button {
                            id: phoneCodeButton

                            width: 150
                            height: parent.height
                            text: qsTr("获取验证码")
                            onClicked: AuthController.showUnsupportedProvider(root.phoneProvider.name)

                            contentItem: Text {
                                text: phoneCodeButton.text
                                color: "white"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.pixelSize: 15
                            }
                            background: Rectangle { radius: 8; color: "#303030" }
                        }
                    }

                    Button {
                        id: phoneLoginButton

                        width: parent.width
                        height: 56
                        text: qsTr("登录")
                        onClicked: AuthController.showUnsupportedProvider(root.phoneProvider.name)
                        contentItem: Text {
                            text: phoneLoginButton.text
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 20
                        }
                        background: Rectangle { radius: 8; color: root.primaryBlue }
                    }
                }

                Column {
                    anchors.fill: parent
                    visible: root.selectedMode === "qr"
                    spacing: 18

                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 180
                        height: 180
                        radius: 10
                        color: "#f6f7f9"
                        border.color: "#e1e4e9"

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("扫码认证\n等待网关提供二维码")
                            color: root.secondaryText
                            horizontalAlignment: Text.AlignHCenter
                            lineHeight: 1.45
                            font.pixelSize: 14
                        }
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("请在手机上确认登录")
                        color: root.primaryText
                        font.pixelSize: 16
                    }

                    Button {
                        id: qrLoginButton

                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 220
                        height: 44
                        text: qsTr("获取扫码认证")
                        onClicked: AuthController.showUnsupportedProvider(root.qrProvider.name)
                        contentItem: Text {
                            text: qrLoginButton.text
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle { radius: 8; color: root.primaryBlue }
                    }
                }

                BusyIndicator {
                    anchors.centerIn: parent
                    running: AuthController.busy && root.selectedMode === ""
                    visible: running
                }
            }

            Item {
                width: parent.width
                height: 42

                Text {
                    anchors.fill: parent
                    visible: AuthController.statusMessage.length > 0
                    text: AuthController.statusMessage
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: {
                        if (AuthController.statusType === "success") {
                            return root.successGreen
                        }
                        if (AuthController.statusType === "error") {
                            return root.errorRed
                        }
                        return root.warningYellow
                    }
                    font.pixelSize: 13
                }
            }

            Row {
                width: parent.width
                spacing: 12
                visible: root.externalProviders.length > 0

                Rectangle { width: (parent.width - 180) / 2; height: 1; anchors.verticalCenter: parent.verticalCenter; color: "#e8eaed" }
                Text { width: 156; text: qsTr("其他登录方式"); horizontalAlignment: Text.AlignHCenter; color: root.secondaryText; font.pixelSize: 14 }
                Rectangle { width: (parent.width - 180) / 2; height: 1; anchors.verticalCenter: parent.verticalCenter; color: "#e8eaed" }
            }

            Item {
                width: parent.width
                height: root.externalProviders.length > 0
                        ? Math.ceil(root.externalProviders.length / 2) * 58
                          + Math.max(0, Math.ceil(root.externalProviders.length / 2) - 1) * 12
                        : 0
                visible: height > 0

                Flow {
                    anchors.fill: parent
                    spacing: 12

                    Repeater {
                        model: root.externalProviders

                        delegate: Button {
                            id: externalButton

                            width: Math.max(160, (parent.width - 12) / 2)
                            height: 58
                            text: modelData.name
                            onClicked: AuthController.showUnsupportedProvider(modelData.name)

                            contentItem: Text {
                                text: externalButton.text
                                color: root.primaryText
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.pixelSize: 14
                            }
                            background: Rectangle {
                                radius: 8
                                color: "white"
                                border.color: "#e1e4e9"
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

    component LoginTab: Button {
        id: loginTab

        property bool active: false

        implicitWidth: tabLabel.implicitWidth
        height: parent.height
        padding: 0
        background: Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 2
            color: loginTab.active ? root.primaryBlue : "transparent"
        }
        contentItem: Text {
            id: tabLabel
            text: loginTab.text
            color: loginTab.active ? root.primaryBlue : "#5e626d"
            font.pixelSize: 18
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    component LoginInput: Rectangle {
        id: loginInput

        property string iconSource
        property string placeholder
        property alias value: input.text
        property alias echoMode: input.echoMode

        height: 58
        radius: 8
        color: "white"
        border.width: 1
        border.color: input.activeFocus ? root.primaryBlue : "#e1e4e9"

        Image {
            anchors.left: parent.left
            anchors.leftMargin: 17
            anchors.verticalCenter: parent.verticalCenter
            source: loginInput.iconSource
            width: 18
            height: 18
            fillMode: Image.PreserveAspectFit
        }

        TextField {
            id: input

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 48
            anchors.rightMargin: 15
            placeholderText: loginInput.placeholder
            placeholderTextColor: "#a5a9b2"
            color: root.primaryText
            font.pixelSize: 15
            selectByMouse: true
            topPadding: 0
            bottomPadding: 0
            leftPadding: 0
            rightPadding: 0
            background: Item {}
        }
    }

    component BrandBadge: Rectangle {
        id: brandBadge

        property string iconSource
        property string text

        width: 164
        height: 52
        radius: 10
        color: "#ffffffcc"

        Row {
            anchors.centerIn: parent
            spacing: 9

            Image {
                source: brandBadge.iconSource
                width: 22
                height: 22
                fillMode: Image.PreserveAspectFit
            }
            Text {
                text: brandBadge.text
                color: "#4d515a"
                font.pixelSize: 13
            }
        }
    }
}

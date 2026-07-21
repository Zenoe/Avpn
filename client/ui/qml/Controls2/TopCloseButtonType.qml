import QtQuick
import QtQuick.Controls
import QtQuick.Shapes

Popup {
    id: root

    property alias buttonWidth: button.implicitWidth

    modal: false
    closePolicy: Popup.NoAutoClose
    padding: 4

    visible: false

    Overlay.modal: Rectangle {
        color: CaelispectStyle.color.translucentMidnightBlack
    }

    background: Rectangle {
        color: CaelispectStyle.color.transparent
    }

    ImageButtonType {
        id: button

        image: "qrc:/images/controls/close.svg"
        imageColor: CaelispectStyle.color.paleGray

        implicitWidth: 40
        implicitHeight: 40

        onClicked: {
            PageController.goToDrawerRootPage()
        }
    }
}

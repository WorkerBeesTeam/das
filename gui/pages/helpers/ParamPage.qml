import QtQuick 2.5
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1

import Das 1.0

Page {
//    default property alias content: pane.contentItem
    default property alias content: contentLayout.children

    property var modelItem
    property var param

    function positionValue(from, to, position) {
        if (typeof to == 'undefined') {
            position = from.position
            to = from.to
            from = from.from
        }

        return from + (position / (1.0 / (to - from)))
    }

    Flickable {
        anchors.top: labelRect.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: btns.top
        anchors.leftMargin: 16
        anchors.rightMargin: 16

        contentHeight: pane.implicitHeight
        flickableDirection: Flickable.AutoFlickIfNeeded

        Pane {
            id: pane
            width: parent.width

            ColumnLayout {
                id: contentLayout
                width: parent.width
                spacing: 20

//                Component.onDestruction: contentLayout.children = ""
            }
        }

        ScrollIndicator.vertical: ScrollIndicator { }
    }

    Rectangle {
        id: labelRect
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: label.height + 16

        color: "#fafafa"

        Label {
            id: label
            text: modelItem ? modelItem.title + " (" + modelItem.sign + ')' : ''

            anchors.centerIn: parent

            font.bold: true
            font.pointSize: 13
        }
    }

    Rectangle {
        id: btns
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        height: okButton.height + 16

        color: "#fafafa"

        Button {
            id: okButton
            anchors.left: parent.left
            anchors.right: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.margins: 8

            text: "Ок"
            font.pointSize: 13
            onClicked: {
                modelItem.param = param
                stackView.pop()
            }

            ToolTip.delay: 1000
            ToolTip.timeout: 5000
            ToolTip.visible: pressed
            ToolTip.text: "Сохранено."
        }

        Button {
            id: cancelButton
            anchors.left: parent.horizontalCenter
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 8

            text: "Отмена"
            font.pointSize: 13
            onClicked: stackView.pop()
        }
    }
}

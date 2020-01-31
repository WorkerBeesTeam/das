import QtQuick 2.5
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
//import QtQuick.VirtualKeyboard 2.1

ColumnLayout {
    id: edit
    property string title: "Дневная"
    property bool title_visible: true

    property real from: 0
    property real to: 50
//    property alias min: fromSpin.from
//    property alias max: toSpin.to

    property alias value1: fromSpin.value
    property alias value2: toSpin.value

    spacing: 6
    Text { text: parent.title; font.bold: true; visible: title_visible }

    width: parent ? parent.width : 0

    Item {
        Layout.fillWidth: true
        height: fromSpin.height

        DialedSpinBox {
            id: fromSpin;
            text: 'От'
            from: edit.from
            to: toSpin.value

            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: -(parent.width / 4)
        }
        DialedSpinBox {
            id: toSpin;
            text: 'До'
            from: fromSpin.value
            to: edit.to

            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: parent.width / 4
        }
    }
}

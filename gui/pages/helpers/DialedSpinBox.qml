import QtQuick 2.5
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1

Item {
    id: dialedSpin
    property alias text: label.text
    property alias from: spin.from
    property alias to: spin.to
    property alias value: spin.value

    height: label.height + dial.height + spin.height + 16
    width: spin.width

    Text {
        id: label
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter

        text: "От"
    }
    Dial {
        id: dial
        anchors.top: label.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 8

        from: spin.from
        to: spin.to
        value: spin.value
        onValueChanged: spin.value = value.toFixed(1)
        onPressedChanged: spin.value = value.toFixed(1)
        onPositionChanged: dialToolTip.text = (from + (to - from) * position).toFixed(1)

        ToolTip {
            id: dialToolTip
            parent: dial.handle
            visible: dial.pressed
            text: dial.value
        }
    }
    DoubleSpinBox {
        id: spin
        anchors.top: dial.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 8

        from: dialedSpin.from
        to: dialedSpin.to
//        value: dialedSpin.value
//        onValueChanged: dialedSpin.value = value
    }
}

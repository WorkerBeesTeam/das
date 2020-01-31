import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.0

import Das 1.0
import "helpers"

SectionsView {
    model: SensorModel {}
    delegate: Item {
        width: parent.width
        height: valueLabel.height + 16

        Text {
            anchors.right: parent.horizontalCenter
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter

            text: model.title
        }
        Text {
            id: valueLabel
            anchors.left: parent.horizontalCenter
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter

            text: model.value + (model.sign ? ' ' + model.sign : '')
        }
    }
}

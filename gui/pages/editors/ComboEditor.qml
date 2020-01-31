import QtQuick 2.5
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import QtQml 2.11

EditorItem {
    name: 'combo editor'
    property var combo_items: ({})

    function initialize() {
    }

    function setAdditionalParams(p) {
        combo_items = p
        control.model.clear()

        for (var key in combo_items) {
            control.model.append({'value': combo_items[key]})
        }

        control.currentIndex = value - 1
    }

    id: stringEditor
    Item {
        id: rect
        anchors.fill: parent
        anchors.margins: -5

        ComboBox {
            id: control
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter

            width: parent.width - 6
            height: parent.height + 2

            model: ListModel {}

            onCurrentIndexChanged: {
                value = currentIndex + 1
                valueEdited()
            }
        }
    }
}

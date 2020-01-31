import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.4
import QtQml 2.11

EditorItem {
    id: boolEditor
    name: 'bool editor'

    function initialize() {
        control.checked = value === true
    }

    Switch {
        id: control


//        indicator: Rectangle {
//            color: checked ? '#0cb80e': 'light grey'
//        }

        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        checked: value === true
        onCheckedChanged: {
            value = checked
            valueEdited()
        }
    }
}

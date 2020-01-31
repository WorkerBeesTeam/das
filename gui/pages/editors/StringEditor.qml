import QtQuick 2.5
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import QtQml 2.11

EditorItem {
    name: 'string editor'

    function initialize() {
        control.text = value
        control.cursorPosition = 0
    }

    id: stringEditor
    Item {
        id: rect
        anchors.fill: parent
        anchors.margins: -5

        TextField {
            id: control
            anchors.centerIn: rect
            Layout.maximumWidth: rect.width
            Layout.minimumWidth: rect.width
            width: rect.width

            activeFocusOnPress: true
            wrapMode: TextEdit.NoWrap
            color: 'black'
            placeholderText: 'Введите текст'

            onTextChanged: {
                value = text
                valueEdited()
            }
        }
    }
}

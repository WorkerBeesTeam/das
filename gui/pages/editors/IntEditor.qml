import QtQuick 2.5
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import QtQml 2.11

NumbersEditorItem {
    name: 'int editor'
    id: intEditor

    maxLimit: 99999
    minLimit: -99999

    function initialize() {
        control.value = getValue()
    }

    Item {
        id: rect
        anchors.fill: parent
        anchors.margins: -5

        SpinBox {
            id: control
            editable: true
            anchors.centerIn: rect
            Layout.maximumWidth: rect.width
            Layout.minimumWidth: rect.width
            width: rect.width

            from: minLimit
            to: maxLimit

            inputMethodHints: Qt.ImhDigitsOnly
            validator: RegExpValidator {
                regExp: /^\d+$/
            }

            onValueModified: {
                setValue(control.value)
                valueEdited()
            }

        }
    }
}

import QtQuick 2.5
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import QtQml 2.11

import "../helpers"

NumbersEditorItem {
    name: 'float editor'
    id: floatEditor

    maxLimit: 99999.0
    minLimit: -99999.0

    function initialize() {
        control.realValue = value
    }

    function isChanged() {
        var diff = Math.abs(value - initial_value)
        return diff >= 0.0009
    }

    Item {
        id: rect
        anchors.fill: parent
        anchors.margins: -5

        DoubleSpinBox {
            id: control

            anchors.centerIn: rect
            Layout.maximumWidth: rect.width
            Layout.minimumWidth: rect.width
            width: rect.width

            realFrom: minLimit
            realTo: maxLimit

            inputMethodHints: Qt.ImhFormattedNumbersOnly

            validator: RegExpValidator {
                regExp: /^[\-]{0,1}\d+([\.\,]{1}\d{0,3}){0,1}$/
            }

            onChanged: {
                setValue(changedValue)
                valueEdited()
            }
        }
    }
}

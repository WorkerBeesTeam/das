import QtQuick 2.5
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import QtQml 2.11

import Das 1.0

EditorItem {
    id: bytesEditor
    name: 'bytes editor'

    property var converter_function: function(s) {return {}}

    function initialize() {
        control.text =  value;
    }

    function saveValues() {
        parameter.setValueFromString(value)
    }

    function updateState() {
        var v = (value.length % 2 == 0) && control.acceptableInput;
        setValid(v);
        rect.border.color = v ? '' : 'red';
        rect.border.width = v ? 0 : 1
        stateChanged();
    }

    function supportsStates() {
        return true;
    }

    function setAdditionalParams(p) {
        converter_function = p
    }

    Rectangle {
        id: rect
        anchors.fill: parent
        anchors.margins: -5
        color: 'transparent'

        TextField {
            id: control
            anchors.centerIn: rect
            Layout.maximumWidth: rect.width
            Layout.minimumWidth: rect.width
            width: rect.width

            activeFocusOnPress: true
            placeholderText: 'Введите байты'
            color: 'black'
            validator: RegExpValidator {
                regExp: /^[0-9a-fA-F]*$/
            }

            onTextChanged: {
                value = text;
                valueEdited()
                updateState();
            }
        }
    }
}

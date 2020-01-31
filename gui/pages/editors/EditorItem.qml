import QtQuick 2.5
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.4
import QtQml 2.11

Item {
    property var initial_value
    property var value
    property string name
    property bool is_valid: true
    property var parameter

    function isValid() { return is_valid; }

    function setValid(validity) { is_valid = validity; }

    function setValue(val) {
        initial_value = val
        value = val
    }

    function setParameter(p) {
        parameter = p
    }

    function getParameter() {
        return parameter
    }

    function setAdditionalParams(p) {
    }

    function initialize() { } //= 0

    function getValue() {
        return value
    }

    function saveValues() {
        parameter.value = value
    }

    function getInitialValue() {
        return initial_value;
    }

    function setName(s) {
        name = s;
    }

    function getName() {
        return name;
    }

    function setIdentifier(s) {
        identifier = s;
    }

    function getIdentifier() {
        return identifier;
    }

    function isChanged() {
        return value != initial_value;
    }

    function supportsStates() {
        return false;
    }

    signal stateChanged();
    signal valueEdited();
}

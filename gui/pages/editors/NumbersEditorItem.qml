import QtQuick 2.5
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import QtQml 2.11

EditorItem {
    property var maxLimit
    property var minLimit

    function setMaxLimit(limit) {
        maxLimit = limit
    }

    function setMinLimit(limit) {
        minLimit = limit
    }
}

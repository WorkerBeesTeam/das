import QtQuick 2.5
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import QtQml 2.11

EditorItem {
    id: notImplemented
    name: 'not implemented widget'

    function isChanged() { return false; }

    Text {
        text: qsTr('Not implemented yet')
    }
}

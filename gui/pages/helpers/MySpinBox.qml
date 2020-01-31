import QtQuick 2.0
import QtQuick.Controls 2.0

SpinBox {
    to: 4000000

    editable: true
    contentItem.onActiveFocusChanged: if (focus) contentItem.selectAll()
    Component.onCompleted: contentItem.inputMethodHints = Qt.ImhDigitsOnly
}

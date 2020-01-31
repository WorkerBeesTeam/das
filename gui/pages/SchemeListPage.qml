import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

Page {
    title: qsTr("Scheme list")

    TextField {
        id: search_field
        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.top: parent.top
        anchors.topMargin: 8
        anchors.right: parent.right
        anchors.rightMargin: 8

        placeholderText: qsTr("Search text")
    }

    ListView {
        anchors.top: search_field.bottom
        anchors.topMargin: 8
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        model: server.schemeListModel
        delegate: ItemDelegate {
            text: title
            width: parent.width
            onClicked: {
                busy_ind.showBusy();
                server.setCurrentScheme(id, name);
            }
        }
    }
}

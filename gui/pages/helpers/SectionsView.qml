import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1

ListView {
    id: sectionView
    anchors.fill: parent
    clip: true

    focus: true

    section {
        criteria: ViewSection.FullString
        property: "name"
        delegate: Rectangle {
            color: "#b0dfb0"
            width: parent.width
            height: sctlbl.height + 16
            Text {
                id: sctlbl
                anchors.centerIn: parent
                font.bold: true
                text: section
            }
        }
    }

    ScrollIndicator.vertical: ScrollIndicator { }
}

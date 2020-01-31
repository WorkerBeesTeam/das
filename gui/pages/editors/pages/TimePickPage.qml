import QtQuick 2.5
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.4
import QtQml 2.11
import QtQuick.Extras 1.4

import "../.."

ScrollablePage {
    id: timePicker

    property int hours: 0
    property int minutes: 0
    property int seconds: 0

    signal accepted
    signal canceled

    function makeNumbers(count) {
        var items = ([])
        for (var i = 0; i < count; ++i) {
            items.push(i.toString())
        }

        return items
    }

    ColumnLayout {
        spacing: 4

        RowLayout {
            Item {
                Layout.fillWidth: true
            }

            Rectangle {
                id: tumblerRect
                width: 300
                height: 300

                Tumbler {
                    id: tumbler
                    anchors.horizontalCenter: parent.horizontalCenter
                    height: parent.height
                    TumblerColumn {
                        id: hoursColumn
                        model: 24
                    }
                    TumblerColumn {
                        id: minutesColumn
                        model: 60
                    }
                    TumblerColumn {
                        id: secondsColumn
                        model: 60
                    }
                    Component.onCompleted: {
                        tumbler.setCurrentIndexAt(0, hours)
                        tumbler.setCurrentIndexAt(1, minutes)
                        tumbler.setCurrentIndexAt(2, seconds)
                    }
                }
            }

            Item {
                Layout.fillWidth: true
            }
        }

        Rectangle {
            id: captionRect
            width: tumblerRect.width
            height: 20
            Layout.alignment: Qt.AlignHCenter

            RowLayout {
                Rectangle {
                    border.width: 1
                    width: captionRect.width / 3
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: 'Час'
                    }
                }
                Rectangle {
                    border.width: 1
                    width: captionRect.width / 3
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: 'Мин'
                    }
                }

                Rectangle {
                    border.width: 1
                    width: captionRect.width / 3
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: 'Сек'
                    }
                }
            }
        }

        // Buttons
        RowLayout {
            Item {
                Layout.fillWidth: true
            }
            Button {
                id: okButton
                text: 'OK'
                onClicked: {
                    hours = hoursColumn.currentIndex
                    minutes = minutesColumn.currentIndex
                    seconds = secondsColumn.currentIndex

                    accepted()
                }
            }
            Item {
                width: 20
            }
            Button {
                id: cancelButton
                text: 'Cancel'
                onClicked: {
                    canceled()
                }
            }
            Item {
                Layout.fillWidth: true
            }
        }
    }
}

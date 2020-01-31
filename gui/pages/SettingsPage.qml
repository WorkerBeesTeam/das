import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

import Das 1.0

ScrollablePage {
    Label {
        text: "Настройки"
        font.bold: true
    }

    ColumnLayout {
        Text { text: "День"; font.bold: true }
        spacing: 6

        RangeSlider {
            id: slider
            to: 23.99
//                    first.value: 7.0
//                    second.value: 20.0
            Layout.fillWidth: true
            snapMode: RangeSlider.SnapAlways

            function pad(n) {
                return (n < 10) ? ("0" + n) : n;
            }

            function setTime(pos, label) {
                var val = pos / (1.0 / 23.99)
                var h = Math.floor(val)
                var m = Math.floor( (val - h) / (1.0 / 60) )
                label.text = pad(h) + ":" + pad(m)
            }

            first.onVisualPositionChanged: setTime( first.visualPosition, fromLabel)
            second.onVisualPositionChanged: setTime( second.visualPosition, toLabel)
        }

        RowLayout {
            Layout.alignment: Qt.AlignCenter
            Text { text: "С:" }
            Text { id: fromLabel; }

            Rectangle { width: 16 }
            Text { text: "По:" }
            Text { id: toLabel; }
        }

        ListView {
            Layout.fillWidth: true
            height: count * 16

            model: NetworksModel{}
            delegate: Text {
                text: name + ": " + value
            }
        }

        ListView {
            Layout.fillWidth: true
            height: count * 16

            model: WiFiModel{}
            delegate: Text {
                text: name
            }
        }
    }
}

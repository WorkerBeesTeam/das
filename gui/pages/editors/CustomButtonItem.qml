import QtQuick 2.5
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.4
import QtQml 2.11

EditorItem {
    name: 'custom button'

    property var mouseClickedHandler: function () {}

    property var secondsToTime: function (sec_num) {
        var hours = Math.floor(sec_num / 3600)
        var minutes = Math.floor((sec_num - (hours * 3600)) / 60)
        var seconds = sec_num - (hours * 3600) - (minutes * 60)

        if (hours < 10) {
            hours = '0' + hours
        }
        if (minutes < 10) {
            minutes = '0' + minutes
        }
        if (seconds < 10) {
            seconds = '0' + seconds
        }

        return hours + ':' + minutes + ':' + seconds
    }

    property var floatToString: function(num) {
        return num.toFixed(3)
    }

    function setText(text) {
        control.text = text
    }

    function initialize() {}

    Item {
        id: pressedStyle
        property real underlineWidth: 2.5
        property string underlineColor: '#0cb80e'
    }

    Item {
        id: normalStyle
        property real underlineWidth: 1
        property string underlineColor: 'grey'
    }

    anchors.fill: parent
    Item {
        id: rect
        anchors.fill: parent
        anchors.margins: -4

        property var style: normalStyle

        Canvas {
            id: canvas
            anchors.fill: parent

            onPaint: { // draw line
                var style = parent.style
                var ctx = getContext('2d')
                ctx.reset()

                ctx.lineWidth = style.underlineWidth
                ctx.strokeStyle = style.underlineColor

                ctx.beginPath()
                ctx.moveTo(0, parent.height - 0.5)
                ctx.lineTo(parent.width, parent.height - 0.5)
                ctx.closePath()
                ctx.stroke()
            }
        }

        TextInput {
            id: control
            anchors.centerIn: parent
            Layout.fillHeight: true
            Layout.fillWidth: true
            activeFocusOnPress: true
            wrapMode: TextEdit.NoWrap
            color: 'black'
            readOnly: true

            MouseArea {
                id: mouseArea
                hoverEnabled: true
                anchors.fill: parent
                onPressed: {
                    rect.style = pressedStyle
                    canvas.requestPaint()
                }

                onReleased: {
                    rect.style = normalStyle
                    canvas.requestPaint()
                }
                onFocusChanged: {
                    rect.style = normalStyle
                    canvas.requestPaint()
                }

                onClicked: mouseClickedHandler()
            }
        }
    }
}

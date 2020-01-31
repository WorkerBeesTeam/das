import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Styles 1.4

Item {
    property string item_date
    property string item_user_name
    property string item_font_color
    property string item_category
    property string item_message

    property int line_size: 16
    property int item_spacing: 4

    id: miniItem
    width: parent.width

    height: line_size * 1.5
    clip: true
    Layout.alignment: Qt.AlignTop

    RowLayout {
        Layout.alignment: Qt.AlignTop
        width: parent.width
        height: line_size
        spacing: item_spacing
        Text {
            font.bold: true
            font.pixelSize: line_size
            text: item_date
        }
        Image {
            width: line_size
            height: line_size
            fillMode: Image.Stretch
            id: user_icon
            source: "qrc:/pages/journal/user.svg"
            MouseArea {
                id: icon_area
                anchors.fill: parent
            }
            ToolTip.visible: icon_area.pressed && item_user_name.length > 0
            ToolTip.text: item_user_name
        }
        Text {
            font.pixelSize: line_size
            font.bold: true
            width: 100
            elide: Text.ElideRight
            color: item_font_color
            text: item_category
        }

        Text {
            id: message_view
            font.pixelSize: line_size
            Layout.fillWidth: true
            wrapMode: Text.NoWrap
            color: item_font_color
            textFormat: Text.PlainText
            text: metrics.elidedText

            TextMetrics {
                id: metrics
                font: message_view.font
                text: miniItem.item_message
                elide: Text.ElideRight
                elideWidth: message_view.width - 10
            }
        }
    }
}

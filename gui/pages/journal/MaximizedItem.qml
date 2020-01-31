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

    id: maxiItem
    width: parent.width
    height: line_size * 1.5 + item_spacing + item_message_field.height

    Column {
        width: parent.width
        spacing: item_spacing
        RowLayout {
            spacing: item_spacing
            width: parent.width
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
                color: item_font_color
                text: item_category
            }
            Text {Layout.fillWidth: true} // spacer
        }
        Text {
            id: item_message_field
            width: parent.width
            font.pixelSize: line_size
            textFormat: Text.PlainText
            color: item_font_color
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            onWidthChanged: { maxiItem.updateState() }
        }
    }

    function updateState() {
        if (visible) {
            item_message_field.text = item_message
        }
    }

    onVisibleChanged: {
        updateState()
    }
}

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Styles 1.4
import QtQuick.Window 2.11
import QtQml 2.11

import Das 1.0

import "journal"

Page {
    id: journalPage
    anchors.fill: parent

    function colorByType(tp) {
        switch (tp) {
        case 'debug':
            return '#ADDFAD' // light green
        case 'info':
            return '#AFEEEE' // light blue
        case 'warning':
            return '#EEE6A3' // blond
        case 'critical':
            return '#F9BDC2' // light red
        case 'fatal':
            return '#E32636' // dark red
        case 'user':
            return 'white'
        default:
            break
        }

        return '#F38EF5' // viola for unknown event type
    }

    property bool listHasItems: false
    property bool is_options_panel_visible: false

    function hideOptionsPanel() {
        is_options_panel_visible = false
    }

    ColumnLayout {
        spacing: 0
        anchors.fill: parent

        Item {
            id: pullDownMenu
            Layout.fillWidth: true
            height: 30
            visible: !journalPage.is_options_panel_visible
            ToolButton {
                anchors.fill: parent
                onClicked: is_options_panel_visible = true
                text: "Показать опции"
            }
        }

        Item {
            id: grid
            visible: journalPage.is_options_panel_visible
            Layout.fillWidth: true
            height: 130

            GridLayout {
                width: parent.width
                columns: 3
                columnSpacing: 4
                rowSpacing: 4

                Text {
                    Layout.alignment: Qt.AlignCenter
                    text: 'Команда'
                    width: 200
                }
                TextField {
                    id: commandField
                    Layout.alignment: Qt.AlignCenter
                    Layout.fillWidth: true
//                    width: 200
                    placeholderText: 'команда'
                    onAccepted: {
//                    Keys.onReturnPressed: {
                        console.info('enter pressed: execute command: ' + commandField.text)
                        server.executeCommand(commandField.text)
                    }
                }
                RoundButton {
                    radius: 3
                    height: 20
                    text: 'Выполнить'
                    onClicked: {
                        console.info('execute command: ' + commandField.text)
                        server.executeCommand(commandField.text)
                    }
                }

                Text {
                    id: filterText
                    Layout.alignment: Qt.AlignCenter
                    text: 'Фильтр'
                    width: 200
                }
                TextField {
                    id: filterField
                    Layout.alignment: Qt.AlignCenter
                    Layout.fillWidth: true
//                    width: 200
                    placeholderText: 'фильтр'
                }
                RoundButton {
                    radius: 3
                    height: 20
                    text: 'Применить'
                    onClicked: {
                        console.info('apply filter: ' + filterField.text)
                        listHasItems = journalModel.items_count() > 0
                    }
                }

                Item {
                    Layout.alignment: Qt.AlignCenter
                    width: 200
                }

                CheckBox {
                    id: unfoldCheck
                    Layout.alignment: Qt.AlignLeft
                    text: 'Разворачивать все'
                }
            }
        }

        Item { // spacer
            height: 24
            visible: journalPage.is_options_panel_visible
        }

        Rectangle { // line over the list
            Layout.fillWidth: true
            height: 2
            border.color: '#212121'
        }

        Item {
            id: journalListItem
            Layout.fillHeight: true
            Layout.fillWidth: true
            visible: listHasItems
            clip: true

            ListView {
                id: journalView
                anchors.fill: parent

                signal requestNewItems
                signal requestOldItems

                Layout.alignment: Qt.AlignLeft

                JournalModel {
                    id: journalModel
                }

                model: journalModel

                onModelChanged: {
                    listHasItems = count > 0
                }

                onCountChanged: {
                    console.info('count changed = ' + count)
                    listHasItems = count > 0
                }

                onMovementStarted: {
                    journalPage.hideOptionsPanel()
                }

                onMovementEnded: {
                    console.info('movement ended: position = ' + verticalScroll.position)
                }

                property int topIndex: -1
                property int bottomIndex: -1

                function updateViewPort() {
                    var topItemIndex = indexAt(1, contentY)
                    var bottomItemIndex = indexAt(1, contentY + journalView.height - 1)
                    if (bottomItemIndex < 0)
                        bottomItemIndex = journalView.count - 1

                    journalView.topIndex = topItemIndex
                    journalView.bottomIndex = bottomItemIndex
                }

                ScrollBar.vertical: ScrollBar {
                    id: verticalScroll
                    width: 20
                    onPositionChanged: {
                        parent.updateViewPort()
                        journalModel.positionChanged(position, journalView.topIndex, journalView.bottomIndex)
                    }
                }

                onContentYChanged: {
                    updateViewPort();
                }

                // process flick
                onFlickEnded: {
//                    if (atYEnd) {
//                        requestOldItems()
//                    }

//                    if (atYBeginning) {
//                        requestNewItems()
//                    }
                }

                delegate: Rectangle {
                    id: listItem
                    width: parent.width

                    property string item_color: colorByType(event_type)
                    property bool is_maximized: unfoldCheck.checked

                    function bool_to_int(v) { return v ? 1 : 0 }
                    height: {
                        return bool_to_int(maximized_item.visible) * maximized_item.height +
                                bool_to_int(minimized_item.visible) * minimized_item.height
                    }

                    function switchSize() {
                        is_maximized = !is_maximized
                    }

                    function msec_to_date(v) {
                        var date = new Date(v)
                        if (date.getDate() === Date()) { // date is today
                            return date.toLocaleTimeString()
                        }

                        return date.toLocaleString()
                    }

                    function toNarrowDate(v) {
                        var d = (new Date(v)).toLocaleDateString(Qt.locale("ru_RU"), Locale.NarrowFormat)
                        return d
                    }

                    function isDateToday(v) {
                        return toNarrowDate(new Date(v)) === toNarrowDate(Date.now())
                    }

                    function toNarrowTime(v) {
                        return v.toLocaleTimeString(Qt.locale("ru_RU"), Locale.NarrowFormat)
                    }

                    function toNarrowDateTime(v) {
                        return v.toLocaleString(Qt.locale("ru_RU"), Locale.NarrowFormat)
                    }

                    function formatDate(v) {
                        if (isDateToday(v)) {
                            // show only time
                            return toNarrowTime(v)
                        }

                        return toNarrowDateTime(v)
                    }

                    property string formatted_date: formatDate(new Date(date_msec))

                    color: item_color

                    MouseArea {
                        id: itemArea
                        anchors.fill: parent
                        onClicked: {
                            journalPage.hideOptionsPanel()
                            listItem.switchSize()
                        }
                    }

                    Column {
                        width: parent.width
                        MaximizedItem {
                            id: maximized_item
                            width: parent.width
                            visible: listItem.is_maximized || unfoldCheck.checked
                            item_font_color: 'black'
                            item_date: formatted_date
                            item_category: category
                            item_user_name: user_name
                            item_message: message
                            line_size: 16
                            item_spacing: 4
                        }

                        MinimizedItem {
                            id: minimized_item
                            width: parent.width
                            visible: !maximized_item.visible
                            item_font_color: 'black'
                            item_date: formatted_date
                            item_category: category
                            item_user_name: user_name
                            item_message: message
                            line_size: 16
                            item_spacing: 4
                        }
                    }
                }

                function test_get_new() {
                    console.info('latest items requested')
                }

                function test_get_old() {
                    console.info('more old items requested')
//                    journalModel.requestOldItems()
                }

                Component.onCompleted: {
                    requestNewItems.connect(test_get_new)
                    requestOldItems.connect(test_get_old)
                    listHasItems = journalView.count > 0
                }
            }
//        }
        }

        Rectangle { // line under the list
            Layout.fillWidth: true
            height: 2
            border.color: '#212121'
        }

        Item {
            // no data in list placeholder
            visible: !listHasItems
            Layout.fillHeight: true
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                Text {
                    Layout.alignment: Qt.AlignCenter
                    text: 'В журнале нет записей.'
                }
            }
        }
    }
}

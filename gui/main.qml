import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.0

import "pages/helpers"

ApplicationWindow {
    id: window
    width: 600
    height: 520
    visible: true
    title: "DeviceAccess"

    Shortcut {
        sequence: "Ctrl+Shift+Q"
        context: Qt.ApplicationShortcut
        onActivated: Qt.quit()
    }

    font.pointSize: 13

//    Settings {
//        id: settings
//        property string style: "Universal"
//    }

    header: ToolBar {
        visible: server.authorized
        Material.foreground: "white"
        RowLayout {
            spacing: 20
            anchors.fill: parent

            ToolButton {
                contentItem: Image {
                    fillMode: Image.Pad
                    horizontalAlignment: Image.AlignHCenter
                    verticalAlignment: Image.AlignVCenter
                    source: stackView.depth > 1 ? "images/back.png" : "images/drawer.png"
                }
                onClicked: {
                    if (stackView.depth > 1) {
                        stackView.pop()
//                        listView.currentIndex = -1
                    } else {
                        drawer.open()
                    }
                }
            }

            Label {
                function getText(item)
                {
                    if (stackView.currentItem && stackView.currentItem.title)
                        return stackView.currentItem.title
                    else if (listView.currentItem)
                        return listView.currentItem.text
                    else
                        return window.title
                }

                id: titleLabel
                text: getText()
                font.pixelSize: 20
//                font.bold: true
                elide: Label.ElideRight
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
                Layout.fillWidth: true
            }

            ToolButton {
                contentItem: Image {
                    fillMode: Image.Pad
                    horizontalAlignment: Image.AlignHCenter
                    verticalAlignment: Image.AlignVCenter
                    source: "qrc:/images/menu.png"
                }
                onClicked: optionsMenu.open()

                Menu {
                    id: optionsMenu
                    x: parent.width - width
                    transformOrigin: Menu.TopRight

                    MenuItem {
                        id: menu_item_schemes
                        text: qsTr("Schemes")
                        onTriggered: stackView.replace('qrc:/pages/SchemeListPage.qml')
                    }

                    MenuItem {
                        text: qsTr("Exit")
                        onTriggered:  stackView.replace(stackView.initialItem)
                    }

                    MenuItem {
                        text: "О программе"
                        onTriggered: aboutDialog.open()
                    }
                }
            }
        }
    }

    Drawer {
        id: drawer
        dragMargin: header.visible ? Qt.styleHints.startDragDistance : 0

        width: Math.min(window.width, window.height) / 3 * 2
        height: window.height

        ListView {
            id: listView
            focus: true
            currentIndex: -1
            anchors.fill: parent

            delegate: ItemDelegate {
                width: parent.width
                text: model.title
//                    background: index % 2 ? "steelblue" : "lightsteelblue"
                highlighted: ListView.isCurrentItem
                onClicked: {
                    listView.setItem(index, model)
                    drawer.close()
                }
            }

            function setIndex(index) {
                setItem(index, model.get(index))
            }

            function setItem(index, item) {
                if (index === 7) aboutDialog.open()
                else if (currentIndex !== index) {
                    currentIndex = index
                    stackView.clear()
                    if (item)
                    {
//                        titleLabel.text = item.title
                        stackView.push(item.source)
                    }
                }
            }

            model: ListModel {
                ListElement { title: "Главная"; source: "qrc:/pages/MainPage.qml" }
                ListElement { title: "Оборудование"; source: "qrc:/pages/EquipmentPage.qml" }
                ListElement { title: "Датчики"; source: "qrc:/pages/SensorPage.qml" }
//                ListElement { title: "Журнал"; source: "qrc:/pages/EventLogPage.qml" }
                ListElement { title: "Журнал"; source: "qrc:/pages/JournalPage.qml" }
                ListElement { title: "Отчёты"; source: "qrc:/pages/ReportPage.qml" }
                ListElement { title: "Структура"; source: "qrc:/pages/StructurePage.qml" }
                ListElement { title: "Настройки"; source: "qrc:/pages/SettingsPage.qml" }
                ListElement { title: "О программе"; source: "qrc:/pages/StructurePage.qml" }
            }

            ScrollIndicator.vertical: ScrollIndicator { }
        }
    }

    Connections {
        target: server
        onAuthorizedChanged: {
            busy_ind.hideBusy();
            menu_item_schemes.triggered()
        }

        onDetailAvailable: {
            busy_ind.hideBusy();
            listView.setIndex(0);
        }
    }

    Item {
        id: appContainer
        anchors.fill: parent

        BusyIndicator {
            id: busy_ind
            visible: false
            readonly property int size: Math.min(parent.width, parent.height) / 1.3
            width: size
            height: size
            anchors.centerIn: parent

            function showBusy() {
                stackView.visible = false
                busy_ind.visible = true
            }

            function hideBusy() {
                busy_ind.visible = false
                stackView.visible = true
            }
        }

        StackView {
            id: stackView
//            visible: false
//            anchors.fill: parent
            initialItem: "qrc:/pages/LoginPage.qml"

            anchors.left: parent.left
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.bottom: with_keyboard && inputPanelL.status === Loader.Ready ? inputPanelL.item.top : parent.bottom

            Component.onCompleted:
                currentItem.loginBegin.connect(busy_ind.showBusy);
        }

        Loader {
            id: inputPanelL
            anchors.fill: parent
            source: with_keyboard ? "tools/MyKeyboard.qml" : ''
        }
    }

    Popup {
        id: aboutDialog
        modal: true
        focus: true
        x: (window.width - width) / 2
        y: window.height / 6
        width: Math.min(window.width, window.height) / 3 * 2
        contentHeight: aboutColumn.height
        closePolicy: Popup.OnEscape | Popup.OnPressOutside

        Column {
            id: aboutColumn
            spacing: 20

            Label {
                text: "О программе"
                font.bold: true
            }

            Label {
                width: aboutDialog.availableWidth
                text: "Телефон службы поддержки: +7 123 456 78 90."
                wrapMode: Label.Wrap
            }
        }
    }

//    Component.onCompleted: listView.setIndex(0)
}

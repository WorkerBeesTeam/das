import QtQuick 2.5
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.4
import QtQml 2.11
import QtQuick.Extras 1.4

import Das 1.0

import "../.."

ScrollablePage {
    id: rangePage

    property var parameter

    property var value_from
    property var value_to
    property var maxLimit
    property var minLimit

    signal accepted;
    signal canceled;

    property var components: ([])

    function updateValues() {
        var first = components[0]
        var second = components[1]

        value_from = first.getValue()
        value_to = second.getValue()

        first.setMinLimit(minLimit)
        first.setMaxLimit(value_to)

        second.setMinLimit(value_from)
        second.setMaxLimit(maxLimit)
    }

    ColumnLayout {
        spacing: 12
        Layout.alignment: Qt.AlignVCenter

        Layout.fillWidth: true

        Repeater {
            id: repeater
            model: 2

            RowLayout {
                spacing: 6
                Layout.fillWidth: true
                RowLayout {
                    Layout.fillHeight: true
                    Text {
                        id: paramTitle

                        MouseArea {
                            id: titleMouseArea
                            anchors.fill: paramTitle
                            hoverEnabled: true

                            ToolTip.text: parameter.get(index).type.description
                            ToolTip.visible: hovered | pressed
                            ToolTip.delay: 100
                            ToolTip.timeout: 2000
                        }

                        //text: parameter.get(index).type.title
                        text: {
                            var t = parameter.get(index).type.title
                            if (t.trim() !== '')
                                return t;

                            switch(index) {
                            case 0:
                                return qsTr('Начальное значение:');
                            case 1:
                                return qsTr('Конечное значение:');
                            default:
                                return 'Введите значение:';
                            }
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                Item {
                    id: loaderRect
                    Layout.maximumWidth: rangePage.width / 2
                    Layout.minimumWidth: rangePage.width / 2
                    width: rangePage.width / 2
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Loader {
                        id: loader1
                        parent: loaderRect
                        anchors.fill: parent
                        source: {
                            if (parameter === null)
                                return Qt.resolvedUrl('/pages/editors/InvalidParameter.qml')

                            var p = parameter.get(index)
                            if (p === null)
                                return Qt.resolvedUrl('/pages/editors/InvalidParameter.qml')

                            switch (p.type.type) {
                            case DIG_Param_Type.IntType:
                                return Qt.resolvedUrl('/pages/editors/IntEditor.qml')
                            case DIG_Param_Type.FloatType:
                                return Qt.resolvedUrl('/pages/editors/FloatEditor.qml')
                            case DIG_Param_Type.TimeType:
                                return Qt.resolvedUrl('/pages/editors/TimeEditor.qml')

                            default:
                                return Qt.resolvedUrl('/pages/editors/NotImplemented.qml');
                            }
                        }

                        onLoaded: {
                            var p = parameter.get(index);
                            item.setValue(p.value);


                            item.setParameter(p);

                            item.initialize();
                            item.valueEdited.connect(updateValues)

                            components.push(item);
                        }
                    }
                }
            }
        }

        // Spacer
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
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

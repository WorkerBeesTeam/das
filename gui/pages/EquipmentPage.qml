import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Styles 1.4

import Das 1.0
import "helpers"

SectionsView {
    id: view
    property int labelWidth: 5

    model: EquipmentModel {}
    delegate: Item {
        anchors.leftMargin: 4
        anchors.left: parent.left
        anchors.rightMargin: 4
        anchors.right: parent.right

        height: grp_view.height + group_item.height + 30

        Rectangle {
            id: box
            anchors.fill: parent
            anchors.bottomMargin: 3
            color: '#D3EFD3'
            radius: 16

            Item {
                id: group_item
                anchors.left: parent.left
                anchors.right: parent.right
                height: autoCheck.height

                Text {
                    id: sctlbl
                    anchors.left: parent.left
                    anchors.leftMargin: 16
                    anchors.verticalCenter: parent.verticalCenter
                    font.bold: true

                    text: model.title
                }
                Switch {
                    id: autoCheck;
                    anchors.right: parent.right
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Авто";

                    checked: model.auto
                    onCheckedChanged: model.auto = checked
                }
            }

            property var grp_model: model.dig_mode_iteml

            ListView {
                id: grp_view
                anchors.top: group_item.bottom
                anchors.left: parent.left
                anchors.right: parent.right

                height: 50 * count
                model: parent.grp_model

                delegate: Item {
                    height: 50
                    anchors.left: parent.left
                    anchors.right: parent.right

                    Rectangle {
                        id: itemRect
                        color: "#F2FBF2";

                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        anchors.topMargin: 2
                        anchors.bottomMargin: 2

                        radius: 42

                        property var want: null

                        function start(value) {
                            if (want == null)
                            {
                                want = value;
                                color = "#399139";
                                toggleTimer.start()
                            }
                            else if (want == value)
                            {
                                toggleTimer.stop();
                                stop();
                            }
                        }

                        function stop() {
                            want = null;
                            color = "#F2FBF2";
                        }

                        Timer {
                            id: toggleTimer
                            interval: 1500
                            onTriggered: parent.stop();
                        }

                        Item {
                            visible: model.isCoils
                            anchors.fill: parent

                            MouseArea {
                                id: coilsArea
                                anchors.fill: parent
                                onClicked: {
                                    var new_val = !model.value;
                                    itemRect.start(new_val);
                                    model.value = new_val;
                                }
                            }

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.right: parent.horizontalCenter
                                text: model.title
                            }

                            Switch {
                                id: coils_switch
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.horizontalCenter

                                enabled: !autoCheck.checked;
                                visible: model.isCoils;

                                checked: model.value
                                onCheckedChanged:  if (model.isCoils && complete) {
                                    itemRect.start(checked);
                                    model.value = checked;
                                }

                                property bool complete: false
                                Component.onCompleted: complete = true
                            }
                        }

                        Item {
                            visible: !model.isCoils
                            anchors.fill: parent

                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.horizontalCenter

                            readonly property int value: model.value
                            onValueChanged:
                                if (!model.isCoils)
                                    slider.value = value;


                            Text {
                                id: notCoilsLabel
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.leftMargin: 16

                                text: model.title
                            }

                            MySpinBox {
                                anchors.left: notCoilsLabel.right
                                anchors.leftMargin: 8
                                id: spin

                                from: 0; to: 100
                                stepSize: 10

                                onValueChanged: {
                                    if (slider.value != value)
                                        slider.value = value
                                    if (model.value !== value)
                                        sliderTimer.restart()
                                }
                            }

                            Slider {
                                id: slider
                                onValueChanged: spin.value = value
                                snapMode: Slider.SnapAlways

                                from: 0; to: 100
                                stepSize: 10

                                anchors.left: spin.right
                                anchors.leftMargin: 8
                                anchors.right: parent.right
                                anchors.rightMargin: 8
                            }

                            Timer {
                                id: sliderTimer
                                interval: 500
                                onTriggered: model.value = spin.value
                            }
                        }
                    }
                }
            }
        }
    }
}

import QtQuick 2.5
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.11

import Das 1.0

ScrollablePage {
    id: paramPage

    property var modelItem
    property var param
    property var param_count: param.count()
    property var sectModel

    property var components: ([]); // array of all comoponents
    property var stateful_components: ([]); // array of components affecting the OK button availability

    function saveValues() {
        var params = [];

        for (var i = 0; i <  components.length; ++i) {
            var c = components[i];
            if (c.isChanged()) {
                // save changes for component
                c.saveValues();
//                console.log('saving state for component ' + c.getName())
//                console.log('initial = <' + c.getInitialValue() + '> value = <' + c.getValue() + '>');
                var p = c.getParameter();
                if (p.type.type === DIG_Param_Type.RangeType) {
                    params.push([p.get(0).id, p.get(0).valueToString()])
                    params.push([p.get(1).id, p.get(1).valueToString()])
                } else
                    params.push([p.id, p.valueToString()]);
            }
        }

        server.set_changed_param_values(params);
    }

    function componentStateChanged(obj, state) {
//        console.log('componentStateChanged');

        var is_valid = true;
        for (var i = 0; i < stateful_components.length; ++i) {
            var c = stateful_components[i];
//            console.log('checking state of component ' + c.getName());
            if (!c.isValid()) {
                is_valid = false;
                break;
            }
        }

        console.log('result = ' + is_valid.toString());

        okButton.enabled = is_valid;
    }

    ColumnLayout {
        spacing: 12
        Layout.alignment: Qt.AlignVCenter

        Layout.fillWidth: true

        Repeater {
            id: repeater
            model: param_count

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

                            ToolTip.text: param.get(index).type.description
                            ToolTip.visible: hovered | pressed
                            ToolTip.delay: 100
                            ToolTip.timeout: 2000
                        }

                        text: param.get(index).type.title
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                Item {
                    id: loaderRect
                    Layout.maximumWidth: paramPage.width / 2
                    Layout.minimumWidth: paramPage.width / 2
                    width: paramPage.width / 2
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Loader {
                        id: loader1
                        parent: loaderRect
                        anchors.fill: parent
                        source: {
                            var p = param.get(index)
                            if (p === null) {
                                return Qt.resolvedUrl('/pages/editors/InvalidParameter.qml')
                            }

                            switch (p.type.type) {
                            case DIG_Param_Type.IntType:
                                return Qt.resolvedUrl('/pages/editors/IntEditor.qml')
                            case DIG_Param_Type.BoolType:
                                return Qt.resolvedUrl('/pages/editors/BoolEditor.qml')
                            case DIG_Param_Type.StringType:
                                return Qt.resolvedUrl('/pages/editors/StringEditor.qml')
                            case DIG_Param_Type.BytesType:
                                return Qt.resolvedUrl('/pages/editors/BytesEditor.qml')
                            case DIG_Param_Type.FloatType:
                                return Qt.resolvedUrl('/pages/editors/FloatEditor.qml')
                            case DIG_Param_Type.TimeType:
                                return Qt.resolvedUrl('/pages/editors/TimeEditor.qml')
                            case DIG_Param_Type.RangeType:
                                return Qt.resolvedUrl('/pages/editors/RangeEditor.qml')
                            case DIG_Param_Type.ComboType:
                                return Qt.resolvedUrl('/pages/editors/ComboEditor.qml')

                            default:
                                return Qt.resolvedUrl('/pages/editors/NotImplemented.qml');
                            }
                        }

                        onLoaded: {
                            var p = param.get(index);
                            switch (p.type.type) {
                            case DIG_Param_Type.BytesType:
                                item.setValue(p.valueToString());
                                item.setAdditionalParams(p.setValueFromString) // converter function
                                break;
                            case DIG_Param_Type.ComboType:
                                item.setValue(p.value);
                                item.setAdditionalParams(sectModel.getComboParamValues(p)) // combo items
                                break;
                            case DIG_Param_Type.RangeType:
                                item.setValue(p.get(0).value)
                                item.setAdditionalParams(p.get(1).value)
                                break;
                            default:
                                item.setValue(p.value);
                            }

                            item.setParameter(p);

                            item.initialize();

                            components.push(item);

//                            console.log('item loaded: ' + item);
                            if (item.supportsStates()) {
                                stateful_components.push(item);
//                                console.log('adding stateful component ' + item + ' name = ' + item.getName());

                                item.stateChanged.connect(componentStateChanged)
                            }
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
                    saveValues()
                    stackView.pop()
                }
            }
            Item {
                width: 20
            }
            Button {
                id: cancelButton
                text: 'Cancel'
                onClicked: stackView.pop()
            }
            Item {
                Layout.fillWidth: true
            }
        }
    }
}

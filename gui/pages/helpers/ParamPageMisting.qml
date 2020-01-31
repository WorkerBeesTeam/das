import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

import Das 1.0

ParamPage {
    param: QuickMistingParam{}

    RowLayout {
        CheckBox {
            id: cb
            text: "Влажность"
            checked: param.humidityChecked
            onCheckedChanged: param.humidityChecked = checked
        }
        MySpinBox {
            id: humidity
            to: 100
            value: param.humidity
            onValueChanged: param.humidity = value
        }
    }

    ColumnLayout {
        spacing: 0
        CheckBox {
            text: "Температура";
            checked: param.temperatureChecked
            onCheckedChanged: param.temperatureChecked = checked
        }
        ColumnLayout {
            spacing: 15
            Layout.leftMargin: 32
            TemperatureEdit {
                title_visible: false

                value1: param.temperatureDown
                onValue1Changed: param.temperatureDown = value1

                value2: param.temperature
                onValue2Changed: param.temperature = value2
            }
        }
    }

    ColumnLayout {
        spacing: 0
        CheckBox {
            text: "Цикличность (час:мин):"
            checked: param.loopChecked
            onCheckedChanged: param.loopChecked = checked
        }
        ColumnLayout {
            spacing: 15
            Layout.leftMargin: 32
            RowLayout {
                Text { text: "Работа"; font: cb.font }

                MySpinBox {
                    value: param.loopWorkHour
                    onValueChanged: param.loopWorkHour = value
                }
                Text { text: ":" }
                MySpinBox {
                    value: param.loopWorkMin
                    onValueChanged: param.loopWorkMin = value
                }
            }

            RowLayout {
                Text { text: "Отдых "; font: cb.font }

                MySpinBox {
                    value: param.loopRestHour
                    onValueChanged: param.loopRestHour = value
                }
                Text { text: ":" }
                MySpinBox {
                    value: param.loopRestMin
                    onValueChanged: param.loopRestMin = value
                }
            }
        }
    }
}

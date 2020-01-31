import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

import Das 1.0

ParamPage {
    param: QuickWateringParam{}

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

    RowLayout {
        Text {
            Layout.fillWidth: true
            text: " Количество раз:"
            font: cb.font
        }
        MySpinBox {
            id: count
            value: param.count
            onValueChanged: param.count = value
        }
    }

    ColumnLayout {
        spacing: 6

        Text {
            Layout.fillWidth: true
            text: " Начиная с (час:мин):"
            font: cb.font
        }
        RowLayout {
            Layout.alignment: Qt.AlignCenter

            MySpinBox {
                value: param.start_hour
                onValueChanged: param.start_hour = value
            }
            Text { text: ":" }
            MySpinBox {
                value: param.start_min
                onValueChanged: param.start_min = value
            }
        }
    }

    ColumnLayout {
        spacing: 6

        Text {
            Layout.fillWidth: true
            text: " Продолжительность (час:мин):"
            font: cb.font
        }
        RowLayout {
            Layout.alignment: Qt.AlignCenter

            MySpinBox {
                value: param.hour
                onValueChanged: param.hour = value
            }
            Text { text: ":" }
            MySpinBox {
                value: param.min
                onValueChanged: param.min = value
            }
        }
    }
}

import QtQuick 2.6
import QtQuick.Layouts 1.3

import QtQuick.Controls 2.0

import Das 1.0

ParamPage {
    RowLayout {
        Layout.alignment: Qt.AlignCenter
        Text { text: "Текущее значение:" }
        Text { text: modelItem.pwm }
    }

    RowLayout {
        Layout.alignment: Qt.AlignCenter
        Text { text: "Пропорциональный" }
        DoubleSpinBox {
            onValueChanged: param.proportional = value
            value: param.proportional
            Component.onCompleted:
                console.log('proportional ' + param.proportional)
        }
    }

    RowLayout {
        Layout.alignment: Qt.AlignCenter
        Text { text: "Интегральный" }
        DoubleSpinBox {
            onValueChanged: param.integral = value
            value: param.integral
        }
    }
    RowLayout {
        Layout.alignment: Qt.AlignCenter
        Text { text: "Дифференцирующий" }
        DoubleSpinBox {
            id: differential

            onValueChanged: param.differential = value
            value: param.differential
        }
    }
    RowLayout {
        Layout.alignment: Qt.AlignCenter
        Text { text: "Период" }
        MySpinBox {
            id: period
            from: 1
            to: 3600

            onValueChanged: param.period = value
            value: param.period
        }
    }

    Rectangle { color: "black"; height: 1; Layout.alignment: Qt.AlignCenter; Layout.fillWidth: true }

    TemperatureEdit {
        from: param.min
        to: param.max

        onValue1Changed: param.dayMin = value1
        value1: param.dayMin

        onValue2Changed: param.dayMax = value2
        value2: param.dayMax
    }

    TemperatureEdit {
        title: "Ночная"

        from: param.min
        to: param.max

        onValue1Changed: param.nightMin = value1
        value1: param.nightMin

        onValue2Changed: param.nightMax = value2
        value2: param.nightMax
    }

    TemperatureEdit {
        title: "Критическая"
        from: -50
        to: 50

        onValue1Changed: param.min = value1
        value1: param.min

        onValue2Changed: param.max = value2
        value2: param.max
    }
}

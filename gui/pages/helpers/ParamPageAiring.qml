import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

import Das 1.0

ParamPage {
    id: page
    param: QuickAiringParam {}

    Slider {
        id: slider1
        Layout.fillWidth: true
        to: 1000
        value: param.co2
        onValueChanged: param.co2 = value

        ToolTip {
            parent: slider1.handle
            visible: slider1.pressed
            text: page.positionValue(slider1).toFixed(0)
        }
    }

    MySpinBox {
        id: fromSpin1
        Layout.alignment: Qt.AlignCenter
        to: 1000
        stepSize: 5

        value: param.co2
        onValueChanged: param.co2 = value
    }

    Text { text: "Температура"; font.bold: true }
    Slider {
        id: slider
        Layout.fillWidth: true
        to: 50
        value: param.temperature
        onValueChanged: param.temperature = value.toFixed(1)

        ToolTip {
            parent: slider.handle
            visible: slider.pressed
            text: page.positionValue(slider).toFixed(1)
        }
    }

    DoubleSpinBox {
        id: fromSpin
        Layout.alignment: Qt.AlignCenter

        value: param.temperature
        onValueChanged: param.temperature = value
    }
}

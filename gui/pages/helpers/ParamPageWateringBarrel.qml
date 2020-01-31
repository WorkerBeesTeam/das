import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

import Das 1.0

ParamPage {
    id: page
    Slider {
        id: slider
        Layout.fillWidth: true
        to: 50
        value: spin.value
        onValueChanged: spin.value = value.toFixed(1)

        ToolTip {
            parent: slider.handle
            visible: slider.pressed
            text: page.positionValue(slider).toFixed(1)
        }
    }

    DoubleSpinBox {
        id: spin
        Layout.alignment: Qt.AlignCenter

        value: param.temperature
        onValueChanged: param.temperature = value
    }
}

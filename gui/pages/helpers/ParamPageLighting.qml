import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

import Das 1.0

ParamPage {
    id: page
    param: QuickLightingParam{}

    ColumnLayout {
        spacing: 6

        RangeSlider {
            id: fsl
            to: 50000
            Layout.fillWidth: true
            snapMode: RangeSlider.SnapAlways

            first.value: param.lamp
            first.onValueChanged: param.lamp = first.value
            second.value: param.curtain
            second.onValueChanged: param.curtain = second.value

            function posValue() {
                return page.positionValue(from, to, first.pressed ? first.position : second.position)
            }

            ToolTip {
                id: dialToolTip
                parent: fsl.first.pressed ? fsl.first.handle :
                        fsl.second.pressed ? fsl.second.handle : null
                visible: fsl.first.pressed || fsl.second.pressed
                text: fsl.posValue().toFixed(0)
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignCenter
            Text { text: "От:" }
            MySpinBox {
                id: lamp;
                value: param.lamp
                onValueChanged: if (!fsl.first.pressed) param.lamp = value
            }

            Rectangle { width: 16 }

            Text { text: "До:" }
            MySpinBox {
                id: curatin;
                value: param.curtain
                onValueChanged: if (!fsl.second.pressed) param.curtain = value
            }
        }
    }

    TemperatureEdit {
        title: "Температура"

        onValue1Changed: param.temperatureDown = value1
        value1: param.temperatureDown

        onValue2Changed: param.temperature = value2
        value2: param.temperature
    }
}

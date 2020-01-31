import QtQuick 2.9
import QtQuick.Controls 2.4

SpinBox{
    property int decimals: 2
    property real realValue: 0.0
    property real realFrom: 0.0
    property real realTo: 100.0
    property real realStepSize: 1.0

    signal changed(real changedValue)

    property real factor: Math.pow(10, decimals)
    id: spinbox
    stepSize: realStepSize*factor
    value: realValue*factor
    to : realTo*factor
    from : realFrom*factor
    validator: DoubleValidator {
        bottom: Math.min(spinbox.from, spinbox.to)*spinbox.factor
        top:  Math.max(spinbox.from, spinbox.to)*spinbox.factor
    }
    textFromValue: function(value, locale) {
        return Number(value / factor).toLocaleString(locale, 'f', decimals)
    }

    valueFromText: function(text, locale) {
        changedValue = Number.fromLocaleString(locale, text)
        changed(changedValue)
        return changedValue * factor
    }

    onValueModified: {
        changedValue = value / factor
        if (realValue !== changedValue)
            changed(changedValue);
    }

    editable: true
}

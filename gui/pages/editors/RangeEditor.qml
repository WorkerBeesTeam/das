import QtQuick 2.5
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.4
import QtQml 2.11

import Das 1.0

CustomButtonItem {
    id: rangeEditor
    name: 'range editor'

    property var value_from
    property var value_to

    property var initial_value_from
    property var initial_value_to

    property var maxLimit
    property var minLimit

    function initialize() {
        setText( rangeToText(value_from, value_to) )
//        console.log('============== initialize ================')
//        console.log('from = ' + parameter.get(0).value)
//        console.log('to   = ' + parameter.get(1).value)

        if (parameter.count() === 4) {
            minLimit = parameter.get(2).value
            maxLimit = parameter.get(3).value
        } else {
            var tp = parameter.get(0).type.type
            switch (tp) {
            case DIG_Param_Type.FloatType:
                minLimit = -99999.0
                maxLimit = 99999.0
                break
            case DIG_Param_Type.IntType:
            case DIG_Param_Type.TimeType:
                minLimit = -99999
                maxLimit = 99999
                break
            default:
                // no limits for unknown control type
                break
            }
        }
    }

    function setAdditionalParams(p) {
        initial_value_from = value
        value_from = value

        initial_value_to = p
        value_to = p
    }

    function saveValues() {
        parameter.get(0).value = value_from
        parameter.get(1).value = value_to
    }

    function isChanged() {
        return value_from != initial_value_from || value_to != initial_value_to;
    }

    function rangeToText(from, to) {
        var format_value = function(v) { return v.toString() }

        switch (parameter.get(0).type.type) {
        case DIG_Param_Type.TimeType:
            format_value = secondsToTime
            break
        case DIG_Param_Type.FloatType:
            format_value = floatToString
            break
        default:
            break
        }

//        console.log('paramtype = ' + parameter.get(0).type.type + ' type = ' + typeof(from) + ' from = ' + from + ' to = ' + to)

        return 'от ' + format_value(from) + ' до ' + format_value(to);
    }

    mouseClickedHandler: function () {
        var frame = stackView.push(Qt.resolvedUrl('/pages/editors/pages/RangePage.qml'), {
                           modelItem: model,
                           title: parameter.title,
                           param: model.param,
                           value_from: value_from,
                           value_to: value_to,
                           parameter: parameter,
                           minLimit: minLimit,
                           maxLimit: maxLimit
                       });

        frame.accepted.connect(function() {
            value_from = frame.value_from
            value_to = frame.value_to
            console.log('saving range: from = ' + value_from + ' to = ' + value_to)
            setText(rangeToText(value_from, value_to))
            valueEdited()
            stackView.pop()
        });

        frame.canceled.connect( function() {
            stackView.pop();
        });
    }
}

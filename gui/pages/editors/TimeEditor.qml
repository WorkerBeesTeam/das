import QtQuick 2.5
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.4
import QtQml 2.11

CustomButtonItem {
    name: 'time editor'
    id: timeEditor

    function initialize() {
        setText( secondsToTime(value) )
    }

    function setMinLimit(limit) {
        // stub
    }

    function setMaxLimit(limit) {
        // stub
    }

    mouseClickedHandler: function () {
        var frame = stackView.push(Qt.resolvedUrl('/pages/editors/pages/TimePickPage.qml'), {
                                       "modelItem": model,
                                       "title": 'Выберите время',
                                       "param": model.param,
                                       "hours": Math.floor(value / 3600),
                                       "minutes": Math.floor(
                                                      (value % 3600) / 60),
                                       "seconds": value % 60
                                   })

        frame.accepted.connect(function () {
            value = frame.hours * 3600 + frame.minutes * 60 + frame.seconds
            setText( secondsToTime(value) )
            valueEdited();
            stackView.pop()
        })

        frame.canceled.connect(function () {
            stackView.pop()
        })
    }
}

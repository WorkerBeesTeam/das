/*  Keyboard input panel.
    The keyboard is anchored to the bottom of the application.
*/
import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.0
import QtQuick.VirtualKeyboard 2.1

import "../pages/helpers"

InputPanel {
    id: inputPanel

    z: 99
    y: appContainer.height
    anchors.left: parent.left
    anchors.right: parent.right
    states: State {
        name: "visible"
        /*  The visibility of the InputPanel can be bound to the Qt.inputMethod.visible property,
            but then the handwriting input panel and the keyboard input panel can be visible
            at the same time. Here the visibility is bound to InputPanel.active property instead,
            which allows the handwriting panel to control the visibility when necessary.
        */
        when: inputPanel.active
        PropertyChanges {
            target: inputPanel
            y: appContainer.height - inputPanel.height
        }
    }
    transitions: Transition {
        from: ""
        to: "visible"
        reversible: true
        ParallelAnimation {
            NumberAnimation {
                properties: "y"
                duration: 250
                easing.type: Easing.InOutQuad
            }
        }
    }

    AutoScroller {}
}

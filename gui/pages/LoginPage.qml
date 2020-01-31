import QtQuick 2.9
import QtQuick.Controls 2.2

Page {
    signal loginBegin()
    Component.onCompleted:
        login_btn.clicked.connect(loginBegin)

    Connections {
        target: server
        onSetAuthData: {
            usrname.text = username
            pwd.text = password
            if (username.length && password.length)
                login_btn.clicked()
        }
    }

    TextField {
        id: usrname
        width: 300
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 8

        inputMethodHints: Qt.ImhNoAutoUppercase
        placeholderText: qsTr("Username")
    }

    TextField {
        id: pwd
        inputMethodHints: Qt.ImhHiddenText
        echoMode: TextInput.Password
        width: 300
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: usrname.bottom
        anchors.topMargin: 8

        placeholderText: qsTr("Password")
    }

    CheckBox {
        id: remember_me
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: pwd.bottom
        anchors.topMargin: 8
        text: qsTr("Remember me (Totally insecure)")
    }

    Button {
        id: login_btn
        enabled: usrname.text.length && pwd.text.length
        width: 100
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: remember_me.bottom
        anchors.topMargin: 8
        text: qsTr("Log in")
        onClicked:
            server.auth(usrname.text, pwd.text, remember_me.checked);
    }
}

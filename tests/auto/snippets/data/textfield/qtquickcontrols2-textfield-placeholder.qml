import QtQuick 2.0
import QtQuick.Controls 2.0

TextField {
    width: 80
    text: "TextField"
    Rectangle {
        anchors.fill: placeholder
        color: 'transparent'
        border.color: 'red'
    }
}

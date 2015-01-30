/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.4
import QtQuick.Controls 2.0

AbstractProgressBar {
    id: control

    implicitWidth: Math.max(background ? background.implicitWidth : 0,
                            indicator ? indicator.implicitWidth : 0) + padding.left + padding.right
    implicitHeight: Math.max(background ? background.implicitHeight : 0,
                             indicator ? indicator.implicitHeight : 0) + padding.top + padding.bottom

    Accessible.role: Accessible.ProgressBar

    padding { top: style.padding; left: style.padding; right: style.padding; bottom: style.padding }

    indicator: Item {
        x: padding.left
        y: padding.top
        width: parent.width - padding.left - padding.right
        height: parent.height - padding.top - padding.bottom
        scale: control.effectiveLayoutDirection === Qt.RightToLeft ? -1 : 1

        Repeater {
            model: indeterminate ? 2 : 1

            Rectangle {
                property real offset: indeterminate ? 0 : control.value

                x: 2 + (indeterminate ? offset * parent.width - 4 : 0)
                y: (parent.height - height) / 2
                width: offset * (parent.width - x) - 2
                height: 2

                color: style.accentColor
                radius: style.roundness

                SequentialAnimation on offset {
                    loops: Animation.Infinite
                    running: indeterminate && visible
                    PauseAnimation { duration: index ? 520 : 0 }
                    NumberAnimation {
                        easing.type: Easing.OutCubic
                        duration: 1240
                        from: 0
                        to: 1
                    }
                    PauseAnimation { duration: index ? 0 : 520 }
                }
            }
        }
    }

    background: Rectangle {
        implicitWidth: 120
        implicitHeight: 6
        x: padding.left
        y: (parent.height - height) / 2
        width: parent.width - padding.left - padding.bottom
        height: 6

        radius: style.roundness
        border.color: style.frameColor
        color: "transparent"
    }
}

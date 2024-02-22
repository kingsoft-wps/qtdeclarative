// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal

Pane {
    padding: 10

//! [1]
Pane {
    Universal.background: Universal.Steel

    Button {
        text: qsTr("Button")
    }
}
//! [1]
}

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

#ifndef QQUICKABSTRACTTEXTAREA_P_H
#define QQUICKABSTRACTTEXTAREA_P_H

#include <QtQuickControls/private/qquickcontrol_p.h>

QT_BEGIN_NAMESPACE

class QQuickText;
class QQuickTextEdit;
class QQuickAbstractTextAreaPrivate;

class Q_QUICKCONTROLS_EXPORT QQuickAbstractTextArea : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged FINAL)
    Q_PROPERTY(QQuickTextEdit *edit READ edit WRITE setEdit NOTIFY editChanged FINAL)
    Q_PROPERTY(QQuickText *placeholder READ placeholder WRITE setPlaceholder NOTIFY placeholderChanged FINAL)

public:
    explicit QQuickAbstractTextArea(QQuickItem *parent = Q_NULLPTR);

    QString text() const;
    void setText(const QString &text);

    QQuickTextEdit *edit() const;
    void setEdit(QQuickTextEdit *edit);

    QQuickText *placeholder() const;
    void setPlaceholder(QQuickText *placeholder);

Q_SIGNALS:
    void textChanged();
    void editChanged();
    void placeholderChanged();

private:
    Q_DISABLE_COPY(QQuickAbstractTextArea)
    Q_DECLARE_PRIVATE(QQuickAbstractTextArea)
};

QT_END_NAMESPACE

#endif // QQUICKABSTRACTTEXTAREA_P_H

/* ======================================================================
    This file is part of ffDiaporama
    ffDiaporama is a tool to make diaporama as video
    Copyright (C) 2011-2014 Dominique Levray <domledom@laposte.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
   ====================================================================== */

#ifndef QMOVIELABEL_H
#define QMOVIELABEL_H

#include "engine/_GlobalDefines.h"
#include <QLabel>
#include <QMouseEvent>

class QMovieLabel : public QLabel {
Q_OBJECT
public:
    QImage          CurrentImage;

    explicit        QMovieLabel(QWidget *parent=0);
                    ~QMovieLabel();

    void            SetImage(QImage Image);
    void            SetImage(QImage *Image);

protected:
    virtual void    resizeEvent(QResizeEvent *);
    virtual void    paintEvent(QPaintEvent *event);
    virtual void    mouseDoubleClickEvent(QMouseEvent *e);
    virtual void    mouseReleaseEvent(QMouseEvent *e);

signals:
    void            DoubleClick();
    void            RightClickEvent(QMouseEvent *);
};

#endif // QMOVIELABEL_H

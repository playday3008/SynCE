/***************************************************************************
 * Copyright (c) 2003 Volker Christian <voc@users.sourceforge.net>         *
 *                                                                         *
 * Permission is hereby granted, free of charge, to any person obtaining a *
 * copy of this software and associated documentation files (the           *
 * "Software"), to deal in the Software without restriction, including     *
 * without limitation the rights to use, copy, modify, merge, publish,     *
 * distribute, sublicense, and/or sell copies of the Software, and to      *
 * permit persons to whom the Software is furnished to do so, subject to   *
 * the following conditions:                                               *
 *                                                                         *
 * The above copyright notice and this permission notice shall be included *
 * in all copies or substantial portions of the Software.                  *
 *                                                                         *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF              *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY    *
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,    *
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE       *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                  *
 ***************************************************************************/
#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <qwidget.h>
#include <qimage.h>
#include <qpixmap.h>
#include <kdebug.h>
#include <qpainter.h>

/**
@author Volker Christian
*/
class ImageViewer : public QWidget
{
Q_OBJECT
public:
    ImageViewer(QWidget *parent = 0, const char *name = 0, WFlags f = 0);
    ~ImageViewer();
    void drawImage(uchar *data, size_t size);
    void paintEvent(QPaintEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void wheelEvent(QWheelEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void keyReleaseEvent(QKeyEvent *e);
    void enterEvent(QEvent *e);
    void leaveEvent(QEvent *e);

signals:
    void mousePressed(ButtonState button, int x, int y);
    void mouseReleased(ButtonState button, int x, int y);
    void mouseMoved(ButtonState button, int x, int y);
    void wheelRolled(int delta);
    void keyPressed(int ascii, int code);
    void keyReleased(int ascii, int code);

public slots:
    void printImage();
    void saveImage();
    void setPdaSize(int x, int y);

private:
    QImage image;
    QPixmap pm;
    ButtonState currentButton;
    QPainter painter;
};

#endif

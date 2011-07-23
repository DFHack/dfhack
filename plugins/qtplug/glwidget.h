/*
 * Copyright (c) 2010 Petr Mrázek (peterix)
 * See LICENSE for details.
 */

#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QGLWidget>

class GLWPrivate;
class GLWidget : public QGLWidget
{
public:
    GLWidget();
    ~GLWidget();

    float rot;
    void resizeGL(int width, int height);

protected:
    void initializeGL();
    void paintGL();
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void timerEvent(QTimerEvent *event);

private:
    GLWPrivate * d;
};

#endif // GLWIDGET_H

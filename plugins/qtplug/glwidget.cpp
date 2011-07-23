/*
 * Copyright (c) 2010 Petr Mrázek (peterix)
 * See LICENSE for details.
 */

#include "glwidget.h"
#include <QtOpenGL>
#include <QGLBuffer>
#include <QGLShaderProgram>
#include <QGLPixelBuffer>
#include <iostream>
#include <GL/gl.h>

struct Vertex
{
    float texcoord[2];
    float color[3];
    float position[3];
};

#define FRS 0.0625
#define FRX FRS/2

// this is crap
const Vertex house_vert[] =
{
    // walls
    { { 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { -4.0, -4.0, -4.0 } },
    { { 0.0, FRS }, { 0.0, 1.0, 0.0 }, { -4.0, -4.0,  4.0 } },
    { { FRS, FRS }, { 0.0, 1.0, 1.0 }, {  4.0, -4.0,  4.0 } },
    { { FRS, 0.0 }, { 1.0, 0.0, 0.0 }, {  4.0, -4.0, -4.0 } },

    { { 0.0, 0.0 }, { 1.0, 0.0, 1.0 }, { -4.0,  4.0, -4.0 } },
    { { 0.0, FRS }, { 1.0, 1.0, 0.0 }, { -4.0,  4.0,  4.0 } },
    { { FRS, FRS }, { 1.0, 1.0, 1.0 }, {  4.0,  4.0,  4.0 } },
    { { FRS, 0.0 }, { 0.0, 0.0, 1.0 }, {  4.0,  4.0, -4.0 } },

    { { 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { -4.0, -4.0, -4.0 } },
    { { 0.0, FRS }, { 0.0, 1.0, 1.0 }, { -4.0, -4.0,  4.0 } },
    { { FRS, FRS }, { 1.0, 0.0, 0.0 }, { -4.0,  4.0,  4.0 } },
    { { FRS, 0.0 }, { 1.0, 0.0, 1.0 }, { -4.0,  4.0, -4.0 } },

    { { 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, {  4.0, -4.0, -4.0 } },
    { { 0.0, FRS }, { 0.0, 1.0, 1.0 }, {  4.0, -4.0,  4.0 } },
    { { FRS, FRS }, { 1.0, 0.0, 0.0 }, {  4.0,  4.0,  4.0 } },
    { { FRS, 0.0 }, { 1.0, 0.0, 1.0 }, {  4.0,  4.0, -4.0 } },
    // roof
    { { 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, { -4.0,  4.0, -4.0 } },
    { { FRS, 0.0 }, { 0.0, 1.0, 1.0 }, {  4.0,  4.0, -4.0 } },
    { { FRX, FRX }, { 1.0, 1.0, 1.0 }, {  0.0,  9.0,  0.0 } }, 

    { { FRS, 0.0 }, { 1.0, 0.0, 0.0 }, {  4.0,  4.0, -4.0 } },
    { { FRS, FRS }, { 1.0, 1.0, 0.0 }, {  4.0,  4.0,  4.0 } },
    { { FRX, FRX },  { 1.0, 1.0, 1.0 }, {  0.0,  9.0,  0.0 } },

    { { FRS, FRS }, { 0.0, 1.0, 0.0 }, {  4.0,  4.0,  4.0 } },
    { { 0.0, FRS }, { 0.0, 1.0, 1.0 }, { -4.0,  4.0,  4.0 } },
    { { FRX, FRX },  { 1.0, 1.0, 1.0 }, {  0.0,  9.0,  0.0 } },

    { { 0.0, FRS }, { 0.0, 1.0, 0.0 }, { -4.0,  4.0,  4.0 } },
    { { 0.0, 0.0 }, { 1.0, 1.0, 0.0 }, { -4.0,  4.0, -4.0 } },
    { { FRX, FRX },  { 1.0, 1.0, 1.0 }, {  0.0,  9.0,  0.0 } }
};

const unsigned char house_idx[] =
{
    // walls
     0,  1,  2,  0,  2,  3,
     4,  5,  6,  4,  6,  7,
     8,  9, 10,  8, 10, 11,
    12, 13, 14, 12, 14, 15,
    // roof
    16, 17, 18,
    19, 20, 21,
    22, 23, 24,
    25, 26, 27
};

class GLWPrivate
{
public:
    QGLBuffer *VBO;
    QGLBuffer *EBO;
    QGLShaderProgram prog;
    int positionAttrib;
    int colorAttrib;
    int texcoordsAttrib;
    int mvpUniform;
    int textureUniform;
    int terrain;
    float pz,rx,ry;
    QPoint lastMouse;
};

GLWidget::GLWidget()
{
    d = new GLWPrivate;
    d->pz = -140.0f;
    d->rx = d->ry = 0.0f;
    d->VBO = d->EBO = 0;
    startTimer( 10 );
}

GLWidget::~GLWidget()
{
    if(d->VBO) delete d->VBO;
    if(d->EBO) delete d->EBO;
    delete d;
}

const char * VS_src =
"#version 130\n"
"in vec3 position; in vec3 color; uniform mat4 mvp; out vec3 c;"
"in vec2 tc_in; out vec2 coord;"
"void main()"
"{"
    "gl_Position = mvp*vec4(position,1);"
    "c = color;"
    "coord = tc_in;"
"}";

const char * FS_src =
"#version 130\n"
"in vec3 c;"
//"out vec4 gl_FragColor;"
"in vec2 coord; uniform sampler2D tex;"
"void main()"
"{"
//    "gl_FragColor = vec4(c,1);"
//    "gl_FragColor = mix( texture(tex, coord), vec4(c,1), 0.5);"
//    "gl_FragColor = vec4(c,1) - texture(tex, coord);"
    "gl_FragColor = vec4(c,1) * texture(tex, coord);"
"}";

//initialization of OpenGL
void GLWidget::initializeGL()
{
    bool test = 1;
    test &= d->prog.addShaderFromSourceCode(QGLShader::Vertex,VS_src);
    test &= d->prog.addShaderFromSourceCode(QGLShader::Fragment,FS_src);
    test &= d->prog.link();
    if(!test)
        std::cout << "OUCH!" << std::endl;

    d->positionAttrib = d->prog.attributeLocation("position");
    d->colorAttrib = d->prog.attributeLocation("color");
    d->texcoordsAttrib = d->prog.attributeLocation("tc_in");

    d->mvpUniform = d->prog.uniformLocation("mvp");
    d->textureUniform = d->prog.uniformLocation("tex");

    if(d->positionAttrib == -1 || d->colorAttrib == -1 || d->mvpUniform == -1)
        std::cout << "Bad attribs!" << std::endl;

    QGLBuffer &VBO = *(d->VBO = new QGLBuffer(QGLBuffer::VertexBuffer));
    VBO.create();
    VBO.bind();
    VBO.allocate(sizeof(house_vert));
    VBO.write(0,house_vert,sizeof(house_vert));

    QGLBuffer &EBO = *(d->EBO = new QGLBuffer(QGLBuffer::IndexBuffer));
    EBO.create();
    EBO.bind();
    EBO.allocate(sizeof(house_idx));
    EBO.write(0,house_idx,sizeof(house_idx));

    QImage texture;
    texture.load("terrain.png");
    d->terrain = bindTexture(texture);

    glClearColor(0.0f, 0.0f, 0.0f, 0.f);

    glShadeModel( GL_SMOOTH );
    glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
    glEnable( GL_TEXTURE_2D );
    glEnable( GL_DEPTH_TEST );
}

void GLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    d->prog.bind();

    QMatrix4x4 projection;
    QMatrix4x4 mvp;

    //projection.ortho(-10,10,-10,10,1,1000);
    float aspect = (float)width()/(float)height();
    projection.perspective(10,aspect,1,1000);
    mvp = projection;

    mvp.translate(0,0,d->pz);
    mvp.rotate(d->ry,1,0,0);
    mvp.rotate(d->rx,0,1,0);
    d->prog.setUniformValue(d->mvpUniform,mvp);

    //glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, d->terrain);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    d->prog.setUniformValue(d->textureUniform,0);

    d->prog.enableAttributeArray(d->positionAttrib);
    d->prog.enableAttributeArray(d->colorAttrib);
    d->prog.enableAttributeArray(d->texcoordsAttrib);

    d->VBO->bind();

    d->prog.setAttributeBuffer(d->texcoordsAttrib, GL_FLOAT, offsetof(Vertex, texcoord), 2, sizeof(Vertex));
    d->prog.setAttributeBuffer(d->positionAttrib, GL_FLOAT, offsetof(Vertex, position), 3, sizeof(Vertex));
    d->prog.setAttributeBuffer(d->colorAttrib, GL_FLOAT, offsetof(Vertex, color), 3, sizeof(Vertex));

    d->EBO->bind();

    glDrawElements(GL_TRIANGLES, d->EBO->size(), GL_UNSIGNED_BYTE, NULL);

    d->prog.release();
}

void GLWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    d->lastMouse = event->pos();
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - d->lastMouse.x();
    int dy = event->y() - d->lastMouse.y();

    if (event->buttons() & Qt::LeftButton)
    {
        d->rx = d->rx + dx;
        d->ry = d->ry + dy;
    }
    d->lastMouse = event->pos();
}

void GLWidget::timerEvent(QTimerEvent *event)
{
    updateGL();
}

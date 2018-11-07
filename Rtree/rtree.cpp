#include <iostream>
#include <GL/glut.h>
#include "opengl.h"
#include "rtree.h"

#define KEY_ESC 27

double glsize = 600;

const unsigned ndim = 2;            // dimension de r
const unsigned maxn = 25;           // maximo de nodos
const unsigned minn = maxn/2;            // minimo de nodos

typedef Ctree<ndim, maxn, minn> tree; 
tree *tmp;             

using namespace std;

void gldraw(){
    glPointSize(1);
    glBegin(GL_POINTS);
        glColor3d(255,0,0);
        for(unsigned i=0; i<vpoints.size(); i+=2)
            glVertex2d(vpoints[i],vpoints[i+1]);
    glEnd();

    //glLineWidth(1);
    glBegin(GL_LINES);        
        for(unsigned i=0; i<vlines.size(); i+=4){            
            glColor3d(0,255,0);    
            glVertex2d(vlines[i],vlines[i+1]);
            glVertex2d(vlines[i],vlines[i+3]);

            glVertex2d(vlines[i],vlines[i+3]);
            glVertex2d(vlines[i+2],vlines[i+3]);

            glVertex2d(vlines[i+2],vlines[i+3]);
            glVertex2d(vlines[i+2],vlines[i+1]);

            glVertex2d(vlines[i+2],vlines[i+1]);
            glVertex2d(vlines[i],vlines[i+1]);
        }       
    glEnd();
}

void glpaint(){
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
    glOrtho(xmin,xmax,ymin,ymax, -1.0f, 1.0f);
    gldraw();
    glutSwapBuffers();
}

void idle(){    glutPostRedisplay();    }

void init_GL(void) {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
}

GLvoid window_redraw(GLsizei width, GLsizei height){
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(xmin,xmax,ymin,ymax,-1.0f,1.0f);
}

GLvoid window_key(unsigned char key, int x, int y) {
    switch (key) {
        case KEY_ESC:
            exit(0);
            break;
        default:
            break;
    }
}

int main(int argc, char *argv[]){
    tmp = new tree();
    tmp->leer_fichero("crime.csv");
    tmp->guardar_linea();

    double t = 0.005;
    xmin-=t;
    xmax+=t;

    ymin-=t;
    ymax+=t;


    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(glsize, glsize);
    glutInitWindowPosition(50, 50);
    glutCreateWindow("R-Tree");
    init_GL();

    glutDisplayFunc(glpaint);

    glutReshapeFunc(&window_redraw);
    glutKeyboardFunc(&window_key);
    glutIdleFunc(&idle);

    glutMainLoop();

    delete tmp;
    return 0;
}
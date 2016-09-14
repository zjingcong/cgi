// colorwindow.h
// Cem Yuksel

#ifndef _COLORWINDOW_INCLUDED_
#define _COLORWINDOW_INCLUDED_

#include <stdlib.h>
#include <stdio.h>

#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

//int CreateColorWindow ( int parentwindow, int x, int y );
void RGBtoHSV ( unsigned char r, unsigned char g, unsigned char b, float &h, float &s, float &v );
void HSVtoRGB ( float h, float s, float v, unsigned char &r, unsigned char &g, unsigned char &b );


#endif

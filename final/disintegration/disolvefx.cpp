# include <OpenImageIO/imageio.h>

# include <cstdlib>
# include <iostream>
# include <fstream>
# include <string>
# include <stdlib.h>
# include <string>
# include <algorithm>
# include <math.h>
# include <cmath>
# include <iomanip>

# include "disolvefx.h"

# ifdef __APPLE__
#   pragma clang diagnostic ignored "-Wdeprecated-declarations"
#   include <GLUT/glut.h>
# else
#   include <GL/glut.h>
# endif

using namespace std;
OIIO_NAMESPACE_USING

# define SCALE_RATE 0.9
# define LIFE_MAX 20
# define max(x, y) (x > y ? x : y)
# define min(x, y) (x < y ? x : y)


pieceXform::pieceXform(int id, int a, int b, int c, int d, int e)
{
  pieceid = id;
  pxres = a;
  pyres = b;
  px = c;
  py = d;
  life_time = e;

  getinputcorners();
}


void pieceXform::setLifetime(int t) {life_time = t;}


void pieceXform::xformSetting(double vx, double vy, double distance)
{
  v_x = vx;
  v_y = vy;
  perspective_distance = distance;
}


void pieceXform::makehole(unsigned char *outputpixmap, int pic_xres)
{
  if (life_time >= 0)
  {
    for (int row_in = py; row_in < py + pyres; row_in++)
    {
      for (int col_in = px; col_in < px + pxres; col_in++)
      {
        for (int k = 0; k < 4; k++)
        {outputpixmap[(row_in * pic_xres + col_in) * 4 + k] = 0;}
      }
    }
  }
}


void pieceXform::getinputcorners()
{
  inputxycorners[0].x = px;
  inputxycorners[0].y = py;
  inputxycorners[1].x = px;
  inputxycorners[1].y = py + pyres;
  inputxycorners[2].x = px + pxres;
  inputxycorners[2].y = py + pyres;
  inputxycorners[3].x = px + pxres;
  inputxycorners[3].y = py;
}


/*
  calculate forward transform matrix
*/
void pieceXform::generateMatrix(char tag, double p1, double p2)
{
  Matrix3D xform;
  // generate transform matrix
  switch (tag)
  {
    // rotation
    case 'r':
      double theta;
      theta = p1;
      xform[0][0] = xform[1][1] = cos(theta * M_PI / 180);
      xform[0][1] = -sin(theta * M_PI / 180);
      xform[1][0] = sin(theta * M_PI / 180);
      break;
    // scale
    case 's':
      double sx, sy;
      sx = p1;
      sy = p2;
      xform[0][0] = sx;
      xform[1][1] = sy;
      break;
    // translate
    case 't':
      double dx, dy;
      dx = p1;
      dy = p2;
      xform[0][2] = dx;
      xform[1][2] = dy;
      break;
    // flip: if xf = 1 flip horizontal, yf = 1 flip vertical
    case 'f':
      double xf, yf;
      xf = p1;
      yf = p2;
      if (xf == 1)  {xform[0][0] = -1;}
      if (yf == 1)  {xform[1][1] = -1;}
      break;
    // shear
    case 'h':
      double hx, hy;
      hx = p1;
      hy = p2;
      xform[0][1] = hx;
      xform[1][0] = hy;
      break;
    // perspective
    case 'p':
      double perpx, perpy;
      perpx = p1;
      perpy = p2;
      xform[2][0] = perpx;
      xform[2][1] = perpy;
      break;
    default:
      return;
  }
  transMatrix_tmp = xform * transMatrix_tmp;
}


/*
four corners forward warp to make space for output image pixmap
*/
void pieceXform::boundingbox()
{
  Vector2D u0, u1, u2, u3;
  u0.x = 0;
  u0.y = 0;
  u1.x = 0;
  u1.y = pyres;
  u2.x = pxres;
  u2.y = pyres;
  u3.x = pxres;
  u3.y = 0;

  xycorners[0] = transMatrix_tmp * u0;
  xycorners[1] = transMatrix_tmp * u1;
  xycorners[2] = transMatrix_tmp * u2;
  xycorners[3] = transMatrix_tmp * u3;

  double x0, y0, x1, y1, x2, y2, x3, y3, x_min, x_max, y_min, y_max;
  x0 = xycorners[0].x + px;
  y0 = xycorners[0].y + py;
  x1 = xycorners[1].x + px;
  y1 = xycorners[1].y + py;
  x2 = xycorners[2].x + px;
  y2 = xycorners[2].y + py;
  x3 = xycorners[3].x + px;
  y3 = xycorners[3].y + py;

  // calculate output image size
  x_max = max(max(x0, x1), max(x2, x3));
  x_min = min(min(x0, x1), min(x2, x3));
  y_max = max(max(y0, y1), max(y2, y3));
  y_min = min(min(y0, y1), min(y2, y3));

  out_pxres = ceil(x_max - x_min);
  out_pyres = ceil(y_max - y_min);
  out_px = floor(x_min);
  out_py = floor(y_min);

  Matrix3D trans1;
  trans1[0][2] = -px;
  trans1[1][2] = -py;
  transMatrix = transMatrix_tmp * trans1;
  Matrix3D trans2;
  trans2[0][2] = px;
  trans2[1][2] = py;
  transMatrix = trans2 * transMatrix;
}


void pieceXform::pieceUpdate()
{
  transMatrix_tmp.setidentity();
  transMatrix.setidentity();
  life_time++;
}


/*
projective warp inverse map
*/
void pieceXform::inversemap(unsigned char *inputpixmap, unsigned char *outputpixmap, int pic_xres, int pic_yres)
{
  cout << "inversemap" << endl;
  cout << "life_time: " << life_time << endl;
  Matrix3D invMatrix;
  invMatrix = transMatrix.inverse();

  if (life_time >= 0)
  {
    for (int row_out = out_py; row_out < out_py + out_pyres; row_out++)  // output image row
    {
      for (int col_out = out_px; col_out < out_px + out_pxres; col_out++)  // output image col
      {
        // output coordinate
        Vector2D xy;
        xy.x = col_out + 0.5;
        xy.y = row_out + 0.5;

        // inverse mapping
        double u, v;
        u = (invMatrix * xy).x;
        v = (invMatrix * xy).y;
        int row_in, col_in;
        row_in = floor(v);
        col_in = floor(u);

        if (row_in < pic_yres && row_in >= 0 && col_in < pic_xres && col_in >= 0)
        {
          for (int k = 0; k < 4; k++)
          {outputpixmap[(row_out * pic_xres + col_out) * 4 + k] = inputpixmap[(row_in * pic_xres + col_in) * 4 + k];}
          if (outputpixmap[(row_out * pic_xres + col_out) * 4 + 3] == 0)
          {
            for (int k = 0; k < 4; k++)
            {outputpixmap[(row_out * pic_xres + col_out) * 4 + k] = inputpixmap[(row_out * pic_xres + col_out) * 4 + k];}
          }
        }
//        else
//        {
//          for (int k = 0; k < 4; k++)
//          {outputpixmap[(row_out * pic_xres + col_out) * 4 + k] = inputpixmap[(row_out * pic_xres + col_out) * 4 + k];}
//        }
      }
    }
  }
}


void pieceXform::piecemotion(unsigned char *inputpixmap, unsigned char *outputpixmap, int pic_xres, int pic_yres)
{
  if (life_time >= 0)
  {
    makehole(outputpixmap, pic_xres);

    double rotation, scale_factor, transx, transy;

    srand(time(NULL));

    rotation = rand() % (360);
    scale_factor = pow(SCALE_RATE, double(life_time));
    transx = life_time * v_x;
    transy = life_time * v_y;

    cout << "transparameter: " << rotation << " " << scale_factor << " " << transx << " " << transy << " " << perspective_distance << endl;

    generateMatrix('s', scale_factor, scale_factor);
    generateMatrix('r', rotation, 0);
    generateMatrix('t', transx, transy);
    // generateMatrix('p', perspective_distance, 0);
    generateMatrix('h', 0.5, 1);
    boundingbox();
    transMatrix.print();
    inversemap(inputpixmap, outputpixmap, pic_xres, pic_yres);

    cout << "input: " << px << " " << py << " " << pxres << " " << pyres << endl;
    cout << "output: " << out_px << " " << out_py << " " << out_pxres << " " << out_pyres << endl;
  }
}

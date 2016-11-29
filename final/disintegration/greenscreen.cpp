# include <OpenImageIO/imageio.h>

# include <cstdlib>
# include <iostream>
# include <fstream>
# include <string>
# include "greenscreen.h"

# ifdef __APPLE__
#   pragma clang diagnostic ignored "-Wdeprecated-declarations"
#   include <GLUT/glut.h>
# else
#   include <GL/glut.h>
# endif

using namespace std;
OIIO_NAMESPACE_USING

# define maximum(x, y, z) ((x) > (y)? ((x) > (z)? (x) : (z)) : ((y) > (z)? (y) : (z)))
# define minimum(x, y, z) ((x) < (y)? ((x) < (z)? (x) : (z)) : ((y) < (z)? (y) : (z)))


// alphamask generation start
/*
convert RGB color image to HSV color image
  Input RGB color primary values: r, g, and b on scale 0 - 255
  Output HSV colors: h on scale 0-360, s and v on scale 0-1
*/
void RGBtoHSV(int r, int g, int b, double &h, double &s, double &v)
{
  double red, green, blue;
  double max, min, delta;

  red = r / 255.0; green = g / 255.0; blue = b / 255.0;  /* r, g, b to 0 - 1 scale */

  max = maximum(red, green, blue);
  min = minimum(red, green, blue);

  v = max;        /* value is maximum of r, g, b */

  if(max == 0){    /* saturation and hue 0 if value is 0 */
     s = 0;
     h = 0;
  }
  else{
    s = (max - min) / max;           /* saturation is color purity on scale 0 - 1 */

    delta = max - min;
    if(delta == 0)                         /* hue doesn't matter if saturation is 0 */
       h = 0;
    else{
       if(red == max)                    /* otherwise, determine hue on scale 0 - 360 */
          h = (green - blue) / delta;
      else if(green == max)
          h = 2.0 + (blue - red) / delta;
      else /* (green == max) */
          h = 4.0 + (red - green) / delta;

      h = h * 60.0;                       /* change hue to degrees */
      if(h < 0)
          h = h + 360.0;                /* negative hue rotated to equivalent positive around color wheel */
    }
  }
}


void get_thresholds(double &th_hl_1, double &th_hl_2, double &th_s_1, double &th_s_2, double &th_v_1, double &th_v_2, double &th_hh_1,
                    double &th_hh_2)
{
  fstream thresholdsFile("thresholds.txt");
  thresholdsFile >> th_hl_1 >> th_hl_2 >> th_s_1 >> th_s_2 >> th_v_1 >> th_v_2 >> th_hh_1 >> th_hh_2;
}


void alphamask(int xres, int yres, unsigned char *inputpixmap, unsigned char *outputpixmap)
{
  // xres_out = xres;
  // yres_out = yres;

  // get thresholds
  double th_hl_1, th_hl_2, th_s_1, th_s_2, th_v_1, th_v_2, th_hh_1, th_hh_2;
  get_thresholds(th_hl_1, th_hl_2, th_s_1, th_s_2, th_v_1, th_v_2, th_hh_1, th_hh_2);
  cout << th_hh_1 << " " << th_hh_2 << endl;

  // convert rgb color to hsv color
  // outputpixmap = new unsigned char [xres * yres * 4];
  for (int i = 0; i < xres; i++)
  {
    for (int j = 0; j < yres; j++)
    {
      int r, g, b;
      double h, s, v;
      unsigned char alpha;
      r = inputpixmap[(j * xres + i) * 4];
      g = inputpixmap[(j * xres + i) * 4 + 1];
      b = inputpixmap[(j * xres + i) * 4 + 2];
      RGBtoHSV(r, g, b, h, s, v);

      // generate alpha channel mask
      alpha = 255;
      // set alpha value according to hue, together with value and saturation
      if (h < th_hl_2 && h > th_hl_1 && s > th_s_1 && s < th_s_2 && v > th_v_1 && v < th_v_2) {alpha = 0;}
      // set alpha value between 0 to 1
      else
      {
        if (h < th_hh_2 && h > th_hh_1)
        {
          // double mid;
          // mid = (th_hl_2 - th_hl_1) / 2 + th_hl_1;
          if (h <= th_hl_1) {alpha = (h / (th_hl_1 - th_hh_1)) * 255;}
          else {alpha = (h / (th_hh_2 - th_hl_2)) * 255;}
        }
      }

      outputpixmap[(j * xres + i) * 4] = r;
      outputpixmap[(j * xres + i) * 4 + 1] = g;
      outputpixmap[(j * xres + i) * 4 + 2] = b;
      outputpixmap[(j * xres + i) * 4 + 3] = alpha;
    }
  }
}
// alphamask generation end


//// composition start
///*
//generate associated color image
//*/
//void associatedColor(const unsigned char *pixmap, unsigned char *associatedpixmap, int w, int h)
//{
//  for (int i = 0; i < w; i++)
//  {
//    for (int j = 0; j < h; j++)
//    {
//      float r, g, b, alpha, a;
//      r = pixmap[(j * w + i) * 4];
//      g = pixmap[(j * w + i) * 4 + 1];
//      b = pixmap[(j * w + i) * 4 + 2];
//      alpha = pixmap[(j * w + i) * 4 + 3];
//
//      a = float(alpha) / 255;
//      associatedpixmap[(j * w + i) * 4] = float(r) * a;
//      associatedpixmap[(j * w + i) * 4 + 1] = float(g) * a;
//      associatedpixmap[(j * w + i) * 4 + 2] = float(b) * a;
//      associatedpixmap[(j * w + i) * 4 + 3] = alpha;
//    }
//  }
//}
//
//
///*
//over operation
//  front value, back value and alpha value on scale 0-255
//*/
//int over(int front, int back, int alpha)
//{
//  int composedValue;
//  composedValue = float(front) + (1 - (float(alpha) / 255)) * float(back);
//
//  return composedValue;
//}
//
//
///*
//composition
//  convert frontground image to associated color image (no need to do so for background image due to 255 alpha value)
//  and do the over operation between associated frontground image and background image
//*/
//void compose(unsigned char *frontpixmap, unsigned char *backpixmap, int posX, int posY)
//{
//  composedpixmap = new unsigned char [xres * yres * 4];
//  for (int i = 0; i < xres; i++)
//  {
//    for (int j = 0; j < yres; j++)
//    {
//      // get associated frontground image pixel value
//      float frontR = 0;
//      float frontG = 0;
//      float frontB = 0;
//      float frontA = 0;
//      if (i >= posX && i < posX + front_w && j >= posY && j < posY + front_h)
//      {
//        int x, y;
//        x = i - posX;
//        y = j - posY;
//        frontR = frontpixmap[(y * front_w + x) * 4];
//        frontG = frontpixmap[(y * front_w + x) * 4 + 1];
//        frontB = frontpixmap[(y * front_w + x) * 4 + 2];
//        frontA = frontpixmap[(y * front_w + x) * 4 + 3];
//      }
//
//      // get associated background image pixel value (background image alpha = 255)
//      float backR, backG, backB;
//      backR = backpixmap[(j * xres + i) * backchannels];
//      backG = backpixmap[(j * xres + i) * backchannels + 1];
//      backB = backpixmap[(j * xres + i) * backchannels + 2];
//
//      // over operation for each channel value and set alpha value as 255
//      composedpixmap[(j * xres + i) * 4] = over(frontR, backR, frontA);
//      composedpixmap[(j * xres + i) * 4 + 1] = over(frontG, backG, frontA);
//      composedpixmap[(j * xres + i) * 4 + 2] = over(frontB, backB, frontA);
//      composedpixmap[(j * xres + i) * 4 + 3] = 255;
//    }
//  }
//}
//// composition end

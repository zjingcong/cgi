/*
OpenGL and GLUT program to warp an input image with matrix commands or mouse click corner positions, display original ans warpped image, and optionally write out the image to an image file.

The program provide two types of warp operation:
  Projective warp - do translation, scale, shear, flip, rotation, perspective transformation with matrix commands
  Bilinear warp   - do bilinear transformation with matrix commands
  Interactive     - let the user interactively position four corners of the output image in the output window with mouse click
                    the output window for click is 1024x600 and then reshape the size after output image generation

Usage: 
warper input_image_name [output_image_name] [mode]
default mode: projective mode
mode switch:
  -b          bilinear switch - do the bilinear warp instead of a perspective warp
  -i          interactive switch
matrix commands:
  r theta     counter clockwise rotation about image origin, theta in degrees
  s sx sy     scale (watch out for scale by 0!)
  t dx dy     translate
  f xf yf     flip - if xf = 1 flip horizontal, yf = 1 flip vertical
  h hx hy     shear
  p px py     perspective
  d           done

Mouse Response:
  In interactive mode, left click in the output window to position output image corners
  Click order: (0, 0), (0, height), (width, height), (width, 0)
Keyboard Response:
  Press q, Q or exit to quit the program

Jingcong Zhang
jingcoz@g.clemson.edu
2016-10-25
*/

# include <OpenImageIO/imageio.h>
# include <stdlib.h>
# include <cstdlib>
# include <iostream>
# include <fstream>
# include <string>
# include <algorithm>
# include <math.h>
# include <cmath>
# include <iomanip>
# include "matrix.h"

# ifdef __APPLE__
#   pragma clang diagnostic ignored "-Wdeprecated-declarations"
#   include <GLUT/glut.h>
# else
#   include <GL/glut.h>
# endif

using namespace std;
OIIO_NAMESPACE_USING

# define max(x, y) (x > y ? x : y)
# define min(x, y) (x < y ? x : y)

static Matrix3D transMatrix;  // transform matrix for the entire transform
static Matrix3D translation;  // extra translation transform matrix
static unsigned char *inputpixmap;  // input image pixels pixmap
static unsigned char *outputpixmap; // output image pixels pixmap
static string inputImage;  // input image file name
static string outputImage; // output image file name
static int xres, yres;  // input image size: width, height
static int xres_out, yres_out;  // output image size: width, height
static int mode;  // program mode - 0: projective warp (basic requirement), 1: bilinear warp, 2: interactive mode
static Vector2D mouseClickCorners[4];
static int mouse_index = 0;


/*
command line option parser
warper input_image_name [output_image_name] [mode]
default mode: projective mode
mode switch:
  -b          bilinear switch - do the bilinear warp instead of a perspective warp
  -i          interactive switch
matrix commands:
  r theta     counter clockwise rotation about image origin, theta in degrees
  s sx sy     scale (watch out for scale by 0!)
  t dx dy     translate
  f xf yf     flip - if xf = 1 flip horizontal, yf = 1 flip vertical
  h hx hy     shear
  p px py     perspective
  d           done
*/
void helpPrinter()
{
  // print help message
  cout << "Help: " << endl;
  cout << "[Usage] warper input_image_name [output_image_name] [mode]" << endl;
  cout << "--------------------------------------------------------------" << endl;
  cout << "default mode: projective warp" << endl;
  cout << "mode switch: " << endl;
  cout << "\t-b          bilinear switch - do the bilinear warp instead of a perspective warp\n"
       << "\t-i          interactive switch" << endl;
  cout << "matrix commands: " << endl;
  cout << "\tr theta     counter clockwise rotation about image origin, theta in degrees\n"
       << "\ts sx sy     scale (watch out for scale by 0!)\n"
       << "\tt dx dy     translate\n"
       << "\tf xf yf     flip - if xf = 1 flip horizontal, yf = 1 flip vertical\n"
       << "\th hx hy     shear\n"
       << "\tp px py     perspective\n"
       << "\td           done\n" << endl;
}
char **getIter(char** begin, char** end, const std::string& option) {return find(begin, end, option);}
void getCmdOptions(int argc, char **argv, string &inputImage, string &outputImage)
{
  // print help message and exit the program
  if (argc < 2)
  {
    helpPrinter();
    exit(0);
  }
  mode = 0;
  char **iter = getIter(argv, argv + argc, "-b");
  if (iter != argv + argc)  {mode = 1;  cout << "program mode: bilinear warp" << endl;}
  else
  {
    iter = getIter(argv, argv + argc, "-i");
    if (iter != argv + argc)  {mode = 2;  cout << "program mode: interactive" << endl;}
  }
  inputImage = argv[1];
  if (mode == 0)
  {
    cout << "program mode: projective warp" << endl;
    if (argc == 3)  {outputImage = argv[2];}
  }
  else
  {
    if (argc == 4)  {outputImage = argv[2];}
  }
}


/*
matrix command parser
  calculate forward transform matrix
*/
void generateMatrix()
{
  cout << "Please enter matrix commands: " << endl;
  Matrix3D xform;
  char tag;
  cin >> tag;
  while (tag != 'd')
  {
    if (tag != 'r' && tag != 's' && tag != 't' && tag != 'f' && 
        tag != 'h' && tag != 'p' && tag != 'd') 
    {helpPrinter(); exit(0);}
    // generate transform matrix
    switch (tag)
    {
      // rotation
      case 'r':
        double theta;
        cin >> theta;
        xform[0][0] = xform[1][1] = cos(theta * M_PI / 180);
        xform[0][1] = -sin(theta * M_PI / 180);
        xform[1][0] = sin(theta * M_PI / 180);
        break;
      // scale
      case 's':
        double sx, sy;
        cin >> sx >> sy;
        xform[0][0] = sx;
        xform[1][1] = sy;
        break;
      // translate
      case 't':
        double dx, dy;
        cin >> dx >> dy;
        xform[0][2] = dx;
        xform[1][2] = dy;
        break;
      // flip: if xf = 1 flip horizontal, yf = 1 flip vertical
      case 'f':
        double xf, yf;
        cin >> xf >> yf;
        if (xf == 1)  {xform[0][0] = -1;}
        if (yf == 1)  {xform[1][1] = -1;}
        break;
      // shear
      case 'h':
        double hx, hy;
        cin >> hx >> hy;
        xform[0][1] = hx;
        xform[1][0] = hy;
        break;
      // perspective
      case 'p':
        double px, py;
        cin >> px >> py;
        xform[2][0] = px;
        xform[2][1] = py;
        break;
      default:
        return;
    }
    cin >> tag;
  }
  transMatrix = xform * transMatrix;
}


/*
four corners forward warp to make space for output image pixmap
*/
void boundingbox(Vector2D xycorners[])
{
  Vector2D u0, u1, u2, u3;
  u0.x = 0;
  u0.y = 0;
  u1.x = 0;
  u1.y = yres;
  u2.x = xres;
  u2.y = yres;
  u3.x = xres;
  u3.y = 0;

  //Vector2D xy0, xy1, xy2, xy3;
  xycorners[0] = transMatrix * u0;
  xycorners[1] = transMatrix * u1;
  xycorners[2] = transMatrix * u2;
  xycorners[3] = transMatrix * u3;

  double x0, y0, x1, y1, x2, y2, x3, y3, x_min, x_max, y_min, y_max;
  x0 = xycorners[0].x;
  y0 = xycorners[0].y;
  x1 = xycorners[1].x;
  y1 = xycorners[1].y;
  x2 = xycorners[2].x;
  y2 = xycorners[2].y;
  x3 = xycorners[3].x;
  y3 = xycorners[3].y;
  
  // calculate output image size
  x_max = max(max(x0, x1), max(x2, x3));
  x_min = min(min(x0, x1), min(x2, x3));
  y_max = max(max(y0, y1), max(y2, y3));
  y_min = min(min(y0, y1), min(y2, y3));
  xres_out = ceil(x_max - x_min);
  yres_out = ceil(y_max - y_min);
  if (mode != 2)  {cout << "output image size: " << xres_out << "x" << yres_out << endl;}
  
  // calculate extra translation
  // extra translation transform: set x_min and y_min to 0
  translation[0][2] = 0 - x_min;
  translation[1][2] = 0 - y_min;
}


/*
projective warp inverse map
*/
void inversemap()
{
  outputpixmap = new unsigned char [xres_out * yres_out * 4];
  // fill the output image with a clear transparent color(0, 0, 0, 0)
  for (int i = 0; i < xres_out * yres_out * 4; i++) {outputpixmap[i] = 0;}

  Matrix3D invMatrix;
  transMatrix = translation * transMatrix;
  cout << "transform matrix: " << endl;
  transMatrix.print();
  invMatrix = transMatrix.inverse();
  cout << "inverse matrix: " << endl;
  invMatrix.print();

  for (int row_out = 0; row_out < yres_out; row_out++)  // output image row
  {
    for (int col_out = 0; col_out < xres_out; col_out++)  // output image col
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
      
      if (row_in < yres && row_in >= 0 && col_in < xres && col_in >= 0)
      {
        for (int k = 0; k < 4; k++)
        {outputpixmap[(row_out * xres_out + col_out) * 4 + k] = inputpixmap[(row_in * xres + col_in) * 4 + k];}
      }
    }
  }
  cout << "Projective inverse complete." << endl;
  cout << "Press Q or q to quit." << endl;
}


/*
bilinear warp inverse map
*/
void bilinear(Vector2D xycorners[])
{
  outputpixmap = new unsigned char [xres_out * yres_out * 4];
  // fill the output image with a clear transparent color(0, 0, 0, 0)
  for (int i = 0; i < xres_out * yres_out * 4; i++) {outputpixmap[i] = 0;}

  // translate the corners
  for (int i = 0; i < 4; i++) {xycorners[i] = translation * xycorners[i];}

  BilinearCoeffs coeff;
  setbilinear(xres, yres, xycorners, coeff);
  for (int row_out = 0; row_out < yres_out; row_out++)
  {
    for (int col_out = 0; col_out < xres_out; col_out++)
    {
      Vector2D xy, uv;

      xy.x = col_out + 0.5;
      xy.y = row_out + 0.5;
      invbilinear(coeff, xy, uv);

      int row_in, col_in;
      row_in = floor(uv.y);
      col_in = floor(uv.x);

      if (row_in < yres && row_in >= 0 && col_in < xres && col_in >= 0)
      {
        for (int k = 0; k < 4; k++)
        {outputpixmap[(row_out * xres_out + col_out) * 4 + k] = inputpixmap[(row_in * xres + col_in) * 4 + k];}
      }
    }
  }
  cout << "Bilinear inverse complete." << endl;
  cout << "Press Q or q to quit." << endl;
}



/*
interactive mode
  1) calculate transform matrix according to mouse click positions
  2) add extra translation
  3) do the inverse mapping
  4) reshape the output image window
*/
void interactive()
{
  // interMatrix
  // | a b c ||u|   |x|
  // | d e f ||v| = |y|
  // | g h 1 ||1|   |w|
  double x0, x1, x2, x3, y0, y1, y2, y3;
  x0 = mouseClickCorners[0].x;
  y0 = mouseClickCorners[0].y;
  x1 = mouseClickCorners[1].x;
  y1 = mouseClickCorners[1].y;
  x2 = mouseClickCorners[2].x;
  y2 = mouseClickCorners[2].y;
  x3 = mouseClickCorners[3].x;
  y3 = mouseClickCorners[3].y;

  double a, b, c, d, e, f;
  a = y1 - y2;
  b = y3 - y2;
  c = y0 - y2;
  d = x1 - x2;
  e = x3 - x2;
  f = x0 - x2;
  double w1, w3;
  w1 = (f - a * e / b) / (d - a * e / b);
  w3 = (f - c * d / a) / (e - b * d / a);

  transMatrix[0][0] = (x3 * w3 - x0) / xres;
  transMatrix[0][1] = (x1 * w1 - x0) / yres;
  transMatrix[0][2] = x0;
  transMatrix[1][0] = (y3 * w3 - y0) / xres;
  transMatrix[1][1] = (y1 * w1 - y0) / yres;
  transMatrix[1][2] = y0;
  transMatrix[2][0] = (w3 - 1) / xres;
  transMatrix[2][1] = (w1 - 1) / yres;

  // refresh the output image size and calculate extra translation
  Vector2D xycorners[4];
  boundingbox(xycorners);
  // add extra translation
  transMatrix = translation * transMatrix;
  cout << "interMatrix: " << endl;
  transMatrix.print();
  // inverse matrix
  Matrix3D invinterMatrix = transMatrix.inverse();
  cout << "invinterMatrix: " << endl;
  invinterMatrix.print();

  for (int row_out = 0; row_out < yres_out; row_out++)  // output image row
  {
    for (int col_out = 0; col_out < xres_out; col_out++)  // output image col
    {
      // output coordinate
      Vector2D xy;
      xy.x = col_out + 0.5;
      xy.y = row_out + 0.5;

      // inverse mapping
      double u, v;
      u = (invinterMatrix * xy).x;
      v = (invinterMatrix * xy).y;
      int row_in, col_in;
      row_in = floor(v);
      col_in = floor(u);
      
      if (row_in < yres && row_in >= 0 && col_in < xres && col_in >= 0)
      {
        for (int k = 0; k < 4; k++)
        {outputpixmap[(row_out * xres_out + col_out) * 4 + k] = inputpixmap[(row_in * xres + col_in) * 4 + k];}
      }
    }
  }
  cout << "Interactive complete." << endl;
  // resize the window
  glutReshapeWindow(xres_out, yres_out);
  cout << "Press Q or q to quit." << endl;
}


/*
get the image pixmap
*/
void readimage(string infilename)
{
  // read the input image and store as a pixmap
  ImageInput *in = ImageInput::open(infilename);
  if (!in)
  {
    cerr << "Cannot get the input image for " << infilename << ", error = " << geterror() << endl;
    exit(0);
  }
  else
  {
    // get the image size and channels information, allocate space for the image
    const ImageSpec &spec = in -> spec();

    xres = spec.width;
    yres = spec.height;
    int channels = spec.nchannels;
    cout << "input image size: " << xres << "x" << yres << endl;
    cout << "channels: " << channels << endl;

    unsigned char tmppixmap[xres * yres * channels];
    in -> read_image(TypeDesc::UINT8, tmppixmap);

    // convert input image to RGBA image
    inputpixmap = new unsigned char [xres * yres * 4];  // input image pixel map is 4 channels
    for (int row = 0; row < yres; row++)
    {
      for (int col = 0; col < xres; col++)
      {
        switch (channels)
        {
          case 1:
            inputpixmap[(row * xres + col) * 4] = tmppixmap[row * xres + col];
            inputpixmap[(row * xres + col) * 4 + 1] = inputpixmap[(row * xres + col) * 4];
            inputpixmap[(row * xres + col) * 4 + 2] = inputpixmap[(row * xres + col) * 4];
            inputpixmap[(row * xres + col) * 4 + 3] = 255;
            break;
          case 3:
            for (int k = 0; k < 3; k++)
            {inputpixmap[(row * xres + col) * 4 + k] = tmppixmap[(row * xres + col) * 3 + k];}
            inputpixmap[(row * xres + col) * 4 + 3] = 255;
            break;
          case 4:
            for (int k = 0; k < 4; k++)
            {inputpixmap[(row * xres + col) * 4 + k] = tmppixmap[(row * xres + col) * 4 + k];}
            break;
          default:
            return;
        }
      }
    }

    in -> close();  // close the file
    delete in;    // free ImageInput
  }
}


/*
write out the associated color image from image pixel map
*/
void writeimage(string outfilename)
{   
  // create the subclass instance of ImageOutput which can write the right kind of file format
  ImageOutput *out = ImageOutput::create(outfilename);
  if (!out)
  {
    cerr << "Could not create output image for " << outfilename << ", error = " << geterror() << endl;
  }
  else
  {   
    // open and prepare the image file
    ImageSpec spec (xres_out, yres_out, 4, TypeDesc::UINT8);
    out -> open(outfilename, spec);
    // write the entire image
    out -> write_image(TypeDesc::UINT8, outputpixmap);

    // .ppm file: 3 channels
    if (outfilename.substr(outfilename.find_last_of(".") + 1, outfilename.length() - 1) == "ppm")
    {
      unsigned char pixmap[xres_out * yres_out * 3];
      for (int row = 0; row < yres_out; row++)
      {
        for (int col = 0; col < xres_out; col++)
        {
          for (int k = 0; k < 3; k++)
          {pixmap[(row * xres_out + col) * 3 + k] = outputpixmap[(row * xres_out + col) * 4 + k];}
        }
      }
      ImageSpec spec (xres_out, yres_out, 3, TypeDesc::UINT8);
      out -> open(outfilename, spec);
      out -> write_image(TypeDesc::UINT8, pixmap);
    }

    cout << "Write the warped image to image file " << outfilename << endl;
    // close the file and free the ImageOutput I created
    out -> close();
    delete out;
  }
}


/*
display composed associated color image
*/
void display(const unsigned char *pixmap, int w, int h)
{
  unsigned char displaypixmap[w * h * 4];
  // modify the pixmap: upside down the image
  for (int row = 0; row < h; row++)
  {
    for (int col = 0; col < w; col++)
    {
      for (int k = 0; k < 4; k++)
      {displaypixmap[(row * w + col) * 4 + k] = pixmap[((h - 1 - row) * w + col) * 4 + k];}
    }
  }

  // display the pixmap
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glRasterPos2i(0, 0);
  // glDrawPixels writes a block of pixels to the framebuffer
  glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, displaypixmap);

  glFlush();
}
void displayInput() {display(inputpixmap, xres, yres);}
void displayOutput()  {display(outputpixmap, xres_out, yres_out);}


/*
Keyboard Callback Routine: 'q', 'Q' or ESC quit
This routine is called every time a key is pressed on the keyboard
*/
void handleKey(unsigned char key, int x, int y)
{
  switch(key)
  {
    case 'q':		// q - quit
    case 'Q':
    case 27:		// esc - quit
      exit(0);
      
    default:		// not a valid key -- just ignore it
      return;
  }
}


/*
mouse callback
  left click on the output image window to get cornet positions
*/
void mouseClick(int button, int state, int x, int y)
{
  if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
  {
    if (mouse_index < 4)
    {
      // cout << "point" << mouse_index << ": ("<< x << ", " << y << ")" << endl;
      mouseClickCorners[mouse_index].x = x;
      mouseClickCorners[mouse_index].y = y;
    }
    mouse_index++;
    if (mouse_index == 4)
    {
      // mouseClick upside down
      Vector2D tmp1;
      tmp1.x = mouseClickCorners[0].x;
      tmp1.y = mouseClickCorners[0].y;
      mouseClickCorners[0].x = mouseClickCorners[1].x;
      mouseClickCorners[0].y = mouseClickCorners[1].y;
      mouseClickCorners[1].x = tmp1.x;
      mouseClickCorners[1].y = tmp1.y;
      Vector2D tmp2;
      tmp2.x = mouseClickCorners[2].x;
      tmp2.y = mouseClickCorners[2].y;
      mouseClickCorners[2].x = mouseClickCorners[3].x;
      mouseClickCorners[2].y = mouseClickCorners[3].y;
      mouseClickCorners[3].x = tmp2.x;
      mouseClickCorners[3].y = tmp2.y;
      
      // do the transformation and fresh the display
      interactive();
      glutPostRedisplay();
      if (outputImage != "")  {writeimage(outputImage);}
    }
  }
}


/*
Reshape Callback Routine: sets up the viewport and drawing coordinates
*/
void handleReshape(int w, int h, int img_width, int img_height)
{
  float factor = 1;
  // make the image scale down to the largest size when user decrease the size of window
  if (w < img_width || h < img_height)
  {
    float xfactor = w / float(img_width);
    float yfactor = h / float(img_height);
    factor = xfactor;
    if (xfactor > yfactor)  {factor = yfactor;}    // fix the image shape when scale down the image size
    glPixelZoom(factor, factor);
  }
  // make the image remain centered in the window
  glViewport((w - img_width * factor) / 2, (h - img_height * factor) / 2, w, h);
  
  // define the drawing coordinate system on the viewport
  // to be measured in pixels
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, w, 0, h);
  glMatrixMode(GL_MODELVIEW);
}
// handleReshape_in for input window, handleReshape_out for output window: input image size may be different from output image size
void handleReshape_in(int w, int h) {handleReshape(w, h, xres, yres);}
void handleReshape_out(int w, int h)  {handleReshape(w, h, xres_out, yres_out);}


/*
Main program
*/
int main(int argc, char* argv[])
{
  Vector2D xycorners[4];  // output image corner positions

  // command line parser and calculate transform matrix
  getCmdOptions(argc, argv, inputImage, outputImage);
  // read input image
  readimage(inputImage);

  // inverse map
  switch (mode)
  {
    // projective warp
    case 0:
      // calculate transform matrix
      generateMatrix();
      // get four output corner positions
      boundingbox(xycorners);
      inversemap();
      if (outputImage != "") {writeimage(outputImage);}
      break;

    // bilinear warp
    case 1:
      // calculate transform matrix
      generateMatrix();
      // get four output corner positions
      boundingbox(xycorners);
      bilinear(xycorners);
      if (outputImage != "") {writeimage(outputImage);}
      break;

    // interactive
    case 2:
      xres_out = 1024;
      yres_out = 600;
      outputpixmap = new unsigned char [xres_out * yres_out * 4];
      // fill the output image with a clear transparent color(0, 0, 0, 0)
      for (int i = 0; i < xres_out * yres_out * 4; i++) {outputpixmap[i] = 0;}
      cout << "Left click the mouse in the output window to position 4 corners for output image." << endl;
      cout << "Click order: (0, 0), (0, height), (width, height), (width, 0)" << endl;
      break;

    default:
      return 0;
  }
  
  // display input image and output image in seperated windows
  // start up the glut utilities
  glutInit(&argc, argv);
  // create the graphics window, giving width, height, and title text
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  
  // display the input image
  // first window: input image
  glutInitWindowSize(xres, yres);
  glutCreateWindow("Input Image");  
  // set up the callback routines to be called when glutMainLoop() detects an event
  glutDisplayFunc(displayInput);	  // display callback
  glutKeyboardFunc(handleKey);	  // keyboard callback
  glutReshapeFunc(handleReshape_in); // window resize callback
  
  // display the output image
  // second window: output image
  glutInitWindowSize(xres_out, yres_out);
  glutCreateWindow("Output Image");
  // set up the callback routines to be called when glutMainLoop() detects an event
  glutDisplayFunc(displayOutput);	  // display callback
  glutKeyboardFunc(handleKey);	  // keyboard callback
  if (mode == 2)  {glutMouseFunc(mouseClick);}  // mouse callback
  glutReshapeFunc(handleReshape_out); // window resize callback
  
  // Routine that loops forever looking for events. It calls the registered
  // callback routine to handle each event that is detected
  glutMainLoop();

  // release memory
  delete [] inputpixmap;
  delete [] outputpixmap;

  return 0;
}


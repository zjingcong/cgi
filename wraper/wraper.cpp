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
#include "matrix.h"

# ifdef __APPLE__
#   pragma clang diagnostic ignored "-Wdeprecated-declarations"
#   include <GLUT/glut.h>
# else
#   include <GL/glut.h>
# endif

using namespace std;
OIIO_NAMESPACE_USING

static Matrix3D transMatrix;  // transform matrix for the entire transform
static unsigned char *inputpixmap;  // input image pixels pixmap
static unsigned char *outputpixmap; // output image pixels pixmap
static int xres, yres;  // input image size: width, height
static int xres_out, yres_out;  // output image size: width, height


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
    cout << "Input image size: " << xres << "x" << yres << endl;
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

    cout << "Write the wraped image to image file " << outfilename << endl;
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
mouse callback: left click any of the displayed windows to quit
*/
void mouseClick(int button, int state, int x, int y)
{
  if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {exit(0);}
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
command line option parser
wraper input_image_name [output_image_name](optional)
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
  cout << "[Usage] wraper input_image_name [output_image_name](optional)" << endl;
  cout << "--------------------------------------------------------------" << endl;
  cout << "\tr theta     counter clockwise rotation about image origin, theta in degrees\n"
       << "\ts sx sy     scale (watch out for scale by 0!)\n"
       << "\tt dx dy     translate\n"
       << "\tf xf yf     flip - if xf = 1 flip horizontal, yf = 1 flip vertical\n"
       << "\th hx hy     shear\n"
       << "\tp px py     perspective\n"
       << "\td           done\n" << endl;
}
void getCmdOptions(int argc, char **argv, string &inputImage, string &outputImage)
{
  // print help message and exit the program
  if (argc < 2)
  {
    helpPrinter();
    exit(0);
  }
  inputImage = argv[1];
  if (argc == 3) {outputImage = argv[2];}
}


/*
calculate transform matrix
*/
void generateMatrix()
{
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
  cout << "Transform Matrix: " << endl;
  transMatrix.print();
}


void boundingbox()
{
  
}


/*
Main program
*/
int main(int argc, char* argv[])
{
  string inputImage;  // input image file name
  string outputImage; // output image file name

  // command line parser and calculate transform matrix
  getCmdOptions(argc, argv, inputImage, outputImage);
  // read input image
  readimage(inputImage);
  
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
  glutMouseFunc(mouseClick);  // mouse callback
  glutReshapeFunc(handleReshape_in); // window resize callback

  // calculate transform matrix
  generateMatrix();
  // wrap the original image
  // wrapimage(mode, parameter);
  // write out to an output image file
  if (outputImage != "") {writeimage(outputImage);}

  // second window: output image
  glutInitWindowSize(xres_out, yres_out);
  glutCreateWindow("Wrapped Image");
  // set up the callback routines to be called when glutMainLoop() detects an event
  glutDisplayFunc(displayOutput);	  // display callback
  glutMouseFunc(mouseClick);  // mouse callback
  glutReshapeFunc(handleReshape_out); // window resize callback
  
  // Routine that loops forever looking for events. It calls the registered
  // callback routine to handle each event that is detected
  glutMainLoop();

  // release memory
  delete [] inputpixmap;
  delete [] outputpixmap;

  return 0;
}


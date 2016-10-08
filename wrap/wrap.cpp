/*
OpenGL and GLUT program to wrap an input image and optionally write out the image to an image file.

Usage: 
wrap input_image_name [output_image_name]

Mouse Response:
  Left click any of the displayed windows to quit the program.

Jingcong Zhang
jingcoz@g.clemson.edu
2016-10-08
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

# ifdef __APPLE__
#   pragma clang diagnostic ignored "-Wdeprecated-declarations"
#   include <GLUT/glut.h>
# else
#   include <GL/glut.h>
# endif

using namespace std;
OIIO_NAMESPACE_USING


static unsigned char *inputpixmap;  // input image pixels pixmap
static unsigned char *outputpixmap; // output image pixels pixmap
static int xres, yres;  // window size: width, height
static int channels;  // input image channels


void wrapimage()
{
  // outputpixmap = new unsigned char [xres * yres * channels];
  // for (int i; i < xres * yres * channels; i++) {outputpixmap[i] = inputpixmap[i];}
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
    channels = spec.nchannels;

    cout << "Image Size: " << xres << "X" << yres << endl;
    cout << "channels: " << channels << endl;

    inputpixmap = new unsigned char [xres * yres * channels];
    in -> read_image(TypeDesc::UINT8, inputpixmap);

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
    ImageSpec spec (xres, yres, channels, TypeDesc::UINT8);
    out -> open(outfilename, spec);
    // write the entire image
    out -> write_image(TypeDesc::UINT8, outputpixmap);
    cout << "Write the wraped image to image file " << outfilename << endl;
    // close the file and free the ImageOutput I created
    out -> close();

    delete out;
  }
}


/*
display composed associated color image
*/
void display(const unsigned char *pixmap)
{
  unsigned char displaypixmap[xres * yres * 4];
  // modify the pixmap: upside down the image and set the display channel number to 4 
  for (int row = 0; row < yres; row++)
  {
    for (int col = 0; col < xres; col++)
    {
      switch (channels)
      {
        case 1:
          displaypixmap[(row * xres + col) * 4] = pixmap[(yres - 1 - row) * xres + col];
          displaypixmap[(row * xres + col) * 4 + 1] = displaypixmap[(row * xres + col) * 4];
          displaypixmap[(row * xres + col) * 4 + 2] = displaypixmap[(row * xres + col) * 4];
          displaypixmap[(row * xres + col) * 4 + 3] = 255;
          break;
        case 3:
          for (int k = 0; k < 3; k++)
          {displaypixmap[(row * xres + col) * 4 + k] = pixmap[((yres - 1 - row) * xres + col) * 3 + k];}
          displaypixmap[(row * xres + col) * 4 + 3] = 255;
          break;
        case 4:
          for (int k = 0; k < 4; k++)
          {displaypixmap[(row * xres + col) * 4 + k] = pixmap[((yres - 1 - row) * xres + col) * 4 + k];}
          break;
        default:
          return;
      }
    }
  }

  // display the pixmap
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glRasterPos2i(0, 0);
  // glDrawPixels writes a block of pixels to the framebuffer
  glDrawPixels(xres, yres, GL_RGBA, GL_UNSIGNED_BYTE, displaypixmap);

  glFlush();
}
void displayInput() {display(inputpixmap);}
void displayOutput()  {display(outputpixmap);}


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
void handleReshape(int w, int h)
{
  float factor = 1;
  // make the image scale down to the largest size when user decrease the size of window
  if (w < xres || h < yres)
  {
    float xfactor = w / float(xres);
    float yfactor = h / float(yres);
    factor = xfactor;
    if (xfactor > yfactor)  {factor = yfactor;}    // fix the image shape when scale down the image size
    glPixelZoom(factor, factor);
  }
  // make the image remain centered in the window
  glViewport((w - xres * factor) / 2, (h - yres * factor) / 2, w, h);
  
  // define the drawing coordinate system on the viewport
  // to be measured in pixels
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, w, 0, h);
  glMatrixMode(GL_MODELVIEW);
}


/*
command line options parser
  wrap input_image_name [output_image_name](optional)
*/
void getCmdOptions(int argc, char* argv[], string &inputImage, string &outputImage)
{
  if (argc >= 2)
  {
    inputImage = argv[1];
    if (argc > 2) {outputImage = argv[2];}
  }
  // print help message
  else  
  {
    cout << "[HELP]" << endl;
    cout << "[Usage] wrap input_image_name [output_image_name](optional)." << endl;
    exit(0);
  }
}


/*
Main program
*/
int main(int argc, char* argv[])
{
  string inputImage;  // input image file name
  string outputImage; // output image file name

  // command line parser
  getCmdOptions(argc, argv, inputImage, outputImage);
  
  // read input image
  readimage(inputImage);
  // wrap the original image
  wrapimage();
  // write out to an output image file
  if (outputImage != "") {writeimage(outputImage);}
  
  // display input image and output image in seperated windows
  // start up the glut utilities
  glutInit(&argc, argv);
  
  // create the graphics window, giving width, height, and title text
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  glutInitWindowSize(xres, yres);

  // first window: input image
  glutCreateWindow("Input Image");  
  // set up the callback routines to be called when glutMainLoop() detects an event
  glutDisplayFunc(displayInput);	  // display callback
  glutMouseFunc(mouseClick);  // mouse callback
  glutReshapeFunc(handleReshape); // window resize callback

  // second window: output image
  glutCreateWindow("Output Image");  
  // set up the callback routines to be called when glutMainLoop() detects an event
  glutDisplayFunc(displayOutput);	  // display callback
  glutMouseFunc(mouseClick);  // mouse callback
  glutReshapeFunc(handleReshape); // window resize callback
  
  // Routine that loops forever looking for events. It calls the registered
  // callback routine to handle each event that is detected
  glutMainLoop();

  return 0;
}


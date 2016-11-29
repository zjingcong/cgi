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

# include "disolvefx.h"
# include "greenscreen.h"

# ifdef __APPLE__
#   pragma clang diagnostic ignored "-Wdeprecated-declarations"
#   include <GLUT/glut.h>
# else
#   include <GL/glut.h>
# endif

using namespace std;
OIIO_NAMESPACE_USING

# define PIECE_SCALE 10.00
# define max(x, y) (x > y ? x : y)
# define min(x, y) (x < y ? x : y)

static unsigned char *inputpixmap;  // input image pixels pixmap
static unsigned char *outputpixmap; // output image pixels pixmap
static string inputImage;  // input image file name
static string outputImage; // output image file name
static int xres, yres;  // input image size: width, height
static int xres_out, yres_out;  // output image size: width, height
static int img_time;
static pieceXform *piece_list;


void readimage(string infilename);
void writeimage(string outfilename);


void getCmdOptions(int argc, char **argv, string &inputImage, string &outputImage)
{
  if (argc >= 2)
  {
    inputImage = argv[1];
    cout << "Input image: " << inputImage << endl;
  }
  else
  {
    cout << "[HELP]" << endl;
    cout << "disintegration input_image_name" << endl;
  }
}


void preprocessing()
{
  // read input image
  readimage(inputImage);
  // allocate memory for output image
  xres_out = xres;
  yres_out = yres;
  outputpixmap = new unsigned char [xres_out * yres_out * 4];
  // green screen alpha mask generation
  alphamask(xres, yres, inputpixmap, outputpixmap);
  // write out the image
  writeimage("alphamask.png");
  // update display image
  readimage("alphamask.png");
  for (int i = 0; i < xres * yres * 4; i++)
  {outputpixmap[i] = inputpixmap[i];}
}


void disolvepieces()
{
  int x_num, y_num;
  x_num = ceil(xres / double(PIECE_SCALE));
  y_num = ceil(yres / double(PIECE_SCALE));

  cout << "x_num: " << x_num << " y_num: " << y_num << endl;

  piece_list = new pieceXform [x_num * y_num];
  for (int i = 0; i < x_num; ++i)
  {
    for (int j = 0; j < y_num; ++j)
    {
      int px = i * PIECE_SCALE;
      int py = j * PIECE_SCALE;
      int id = j * x_num + i;
      if (i < x_num - 1 && j < y_num - 1)
      {piece_list[id] = pieceXform(id, PIECE_SCALE, PIECE_SCALE, px, py, -1);}
      else
      {
        int pxres = xres - (x_num - 1) * PIECE_SCALE;
        int pyres = yres - (y_num - 1) * PIECE_SCALE;
        piece_list[id] = pieceXform(id, pxres, pyres, px, py, -1);
      }
    }
  }
}


void motionSummary()
{
  for (int i = 0; i < xres * yres * 4; i++)
  {outputpixmap[i] = inputpixmap[i];}

  if (piece_list[8].life_time < 0)  {piece_list[8].setLifetime(1);}
  piece_list[8].xformSetting(10, 10, 0.001);
  // unsigned char input [xres * yres * 4];
  // for (int i = 0; i < xres * yres * 4; ++i) {input[i] = outputpixmap[i];}
  piece_list[8].piecemotion(inputpixmap, outputpixmap, xres, yres);
  piece_list[8].pieceUpdate();
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

    unsigned char displaypixmap[xres * yres * 4];
    // modify the pixmap: upside down the image
    for (int row = 0; row < yres; row++)
    {
      for (int col = 0; col < xres; col++)
      {
        for (int k = 0; k < 4; k++)
        {displaypixmap[(row * xres + col) * 4 + k] = inputpixmap[((yres - 1 - row) * xres + col) * 4 + k];}
      }
    }
    for (int i = 0; i < xres * yres * 4; i++) {inputpixmap[i] = displaypixmap[i];}

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
    // modify the pixmap: upside down the image
    unsigned char outmap [xres_out * yres_out * 4];
    for (int row = 0; row < yres_out; row++)
    {
      for (int col = 0; col < xres_out; col++)
      {
        for (int k = 0; k < 4; k++)
        {outmap[(row * xres + col) * 4 + k] = outputpixmap[((yres - 1 - row) * xres + col) * 4 + k];}
      }
    }

    // open and prepare the image file
    ImageSpec spec (xres_out, yres_out, 4, TypeDesc::UINT8);
    out -> open(outfilename, spec);
    // write the entire image
    out -> write_image(TypeDesc::UINT8, outmap);

    // .ppm file: 3 channels
    if (outfilename.substr(outfilename.find_last_of(".") + 1, outfilename.length() - 1) == "ppm")
    {
      unsigned char pixmap[xres_out * yres_out * 3];
      for (int row = 0; row < yres_out; row++)
      {
        for (int col = 0; col < xres_out; col++)
        {
          for (int k = 0; k < 3; k++)
          {pixmap[(row * xres_out + col) * 3 + k] = outmap[(row * xres_out + col) * 4 + k];}
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
  // display the pixmap
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glRasterPos2i(0, 0);
  // glDrawPixels writes a block of pixels to the framebuffer
  glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixmap);

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

    case 32:
      img_time++;
      cout << "space press" << endl;
      motionSummary();
      glutPostRedisplay();
      break;

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
    exit(0);
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
  // command line parser and calculate transform matrix
  getCmdOptions(argc, argv, inputImage, outputImage);
  preprocessing();
  disolvepieces();

  // display input image and output image in seperated windows
  // start up the glut utilities
  glutInit(&argc, argv);
  // create the graphics window, giving width, height, and title text
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);

  // display the input image
  // first window: input image
  glutInitWindowSize(xres, yres);
  glutCreateWindow("Disintegration Effects");
  // set up the callback routines to be called when glutMainLoop() detects an event
  glutDisplayFunc(displayOutput);	  // display callback
  glutKeyboardFunc(handleKey);	  // keyboard callback
  glutMouseFunc(mouseClick);  // mouse callback
  glutReshapeFunc(handleReshape_in); // window resize callback

//  // display the output image
//  // second window: output image
//  glutInitWindowSize(xres_out, yres_out);
//  glutCreateWindow("Output Image");
//  // set up the callback routines to be called when glutMainLoop() detects an event
//  glutDisplayFunc(displayOutput);	  // display callback
//  glutKeyboardFunc(handleKey);	  // keyboard callback
//  // if (mode == 2)  {glutMouseFunc(mouseClick);}  // mouse callback
//  glutReshapeFunc(handleReshape_out); // window resize callback

  // Routine that loops forever looking for events. It calls the registered
  // callback routine to handle each event that is detected
  glutMainLoop();

  // release memory
  delete [] inputpixmap;
  delete [] outputpixmap;

  return 0;
}

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
# include "time.h"

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

# define PIECE_SCALE 20.00
# define DISOLVE_START_X 100
# define DISOLVE_START_Y 0
# define FRAME_INTERVAL 200

# define max(x, y) (x > y ? x : y)
# define min(x, y) (x < y ? x : y)

static int img_time = -1; // image play time
static int play_mode = 0; // dynamic image play mode: 0 - space key, 1 - auto-play
bool endflag = false;

static unsigned char *inputpixmap;  // input image pixels pixmap
static unsigned char *outputpixmap; // output image pixels pixmap
static unsigned char *frontpixmap;
static unsigned char *backpixmap;
static unsigned char *composedpixmap;  // compose image pixels pixmap

static string inputImage;  // input image file name
static string backImage; // output image file name
static int xres, yres;  // input image size: width, height
static int xres_out, yres_out;  // output image size: width, height

static int x_num, y_num; // piece col number and row number
static pieceXform *piece_list;
static int piece_active_num = 0;
static int active_id_list[1024 * 1024];
static int active_list_index = 0;


void getImageInfo(string infilename);
void readimage(string infilename, unsigned char *inpixmap);
void writeimage(string outfilename);
void composeImage();


char **getIter(char** begin, char** end, const std::string& option) {return find(begin, end, option);}
void getCmdOptions(int argc, char **argv, string &inputImage, string &backImage)
{
  if (argc < 2)
  {
    cout << "[HELP]" << endl;
    cout << "disintegration input_image_name [background_image_name] [play_mode_tag]" << endl;
    cout << "\tdefault background: white" << endl;
    cout << "[play_mode tag]" << endl;
    cout << "\tdefault space key to play the image frames" << endl;
    cout << "\t-a      auto-play the image frames" << endl;
    exit(0);
  }
  char **iter = getIter(argv, argv + argc, "-a");
  if (iter != argv + argc)  {play_mode = 1;  cout << "dynamic image play mode: auto-play" << endl;}
  inputImage = argv[1];
  cout << "Input image: " << inputImage << endl;
  if (argc >= 3 && play_mode == 0)  {backImage = argv[2]; cout << "Background image: " << backImage << endl;}
  if (play_mode == 1 && argc >= 4)  {backImage = argv[2]; cout << "Background image: " << backImage << endl;}
}


void preprocessing()
{
  getImageInfo(inputImage);
  // read input image
  inputpixmap = new unsigned char [xres * yres * 4];
  readimage(inputImage, inputpixmap);
  // allocate memory for output image
  xres_out = xres;
  yres_out = yres;
  outputpixmap = new unsigned char [xres_out * yres_out * 4];
  // green screen alpha mask generation
  alphamask(xres, yres, inputpixmap, outputpixmap);
  // write out the image
  writeimage("alphamask.png");
  // update display image
  readimage("alphamask.png", inputpixmap);
  readimage("alphamask.png", outputpixmap);

  backpixmap = new unsigned char [xres_out * yres_out * 4];
  for (int i = 0; i < xres_out * yres_out * 4; ++i) {backpixmap[i] = 255;}
  if (backImage != "")
  {readimage(backImage, backpixmap);}

  frontpixmap = new unsigned char [xres_out * yres_out * 4];
  composedpixmap = new unsigned char [xres_out * yres_out * 4];
  composeImage();
}


void disolvepieces()
{
  x_num = ceil(xres / double(PIECE_SCALE));
  y_num = ceil(yres / double(PIECE_SCALE));

  piece_list = new pieceXform [x_num * y_num];
  for (int i = 0; i < x_num; ++i)
  {
    for (int j = 0; j < y_num; ++j)
    {
      int px = i * PIECE_SCALE;
      int py = j * PIECE_SCALE;
      int id = j * x_num + i;
      if (i < x_num - 1 && j < y_num - 1)
      {piece_list[id].pieceXformInit(id, i, j, PIECE_SCALE, PIECE_SCALE, px, py, -1);}
      else
      {
        int pxres = xres - (x_num - 1) * PIECE_SCALE;
        int pyres = yres - (y_num - 1) * PIECE_SCALE;
        piece_list[id].pieceXformInit(id, i, j, pxres, pyres, px, py, -1);
      }
    }
  }
}


void motionSummary()
{
  for (int i = 0; i < xres * yres * 4; i++)
  {outputpixmap[i] = inputpixmap[i];}

  // active piece generator
  int new_active_id_list[x_num * y_num];
  int i = 0;
  int new_active_num = 0;
  if (img_time >= 0 && img_time < y_num)
  {
    int y = img_time;
    for (int x = 0; x < x_num; x++)
    {
      new_active_id_list[i] = y * x_num + x;
      active_id_list[active_list_index] = y * x_num + x;

      i++;
      active_list_index++;
      piece_active_num++;
      new_active_num++;
    }
    // start the active piece
    for (int i = 0; i < new_active_num; i++)
    {
      if (piece_list[new_active_id_list[i]].life_time == -1)
      {
        piece_list[new_active_id_list[i]].setLifetime(0);
        piece_list[new_active_id_list[i]].setStarttime(img_time);
      }
    }
  }

  // make hole
  for (int i = 0; i < x_num * y_num; i++)
  {
    if (piece_list[i].life_time != -1 && piece_list[i].pieceid_x < x_num && piece_list[i].pieceid_y < y_num)
    {
      piece_list[i].makehole(outputpixmap, xres);
    }
  }

  // pieces movement
  for (int i = 0; i < x_num * y_num; i++)
  {
    if (piece_list[i].life_time != -1)
    {
      int v_x = rand() % (10) - 5;
      int v_y = rand() % (5) + 5;
      piece_list[i].xformSetting(v_x, v_y, 0);
      piece_list[i].piecemotion(inputpixmap, outputpixmap, xres, yres);
    }
  }

  // clean
  int die_num = 0;
  for (int i = 0; i < piece_active_num; i++)
  {
    if (piece_list[active_id_list[i]].life_time == -2)  {die_num++;}
  }
  if (die_num == piece_active_num)  {endflag = true;}
}


void pieceStatusUpdate()
{
  for (int i = 0; i < x_num * y_num; i++)
  {piece_list[i].pieceUpdate();}
}


void composeImage()
{
  delete [] frontpixmap;
  delete [] composedpixmap;
  delete [] backpixmap;
  frontpixmap = new unsigned char [xres_out * yres_out * 4];
  composedpixmap = new unsigned char [xres_out * yres_out * 4];
  backpixmap = new unsigned char [xres_out * yres_out * 4];
  for (int i = 0; i < xres_out * yres_out * 4; ++i) {backpixmap[i] = 255;}
  if (backImage != "")
  {readimage(backImage, backpixmap);}

  associatedColor(outputpixmap, frontpixmap, xres_out, yres_out);
  compose(composedpixmap, frontpixmap, backpixmap, 0, 0, xres_out, yres_out);
}


void displayFresh()
{
  img_time++;
  motionSummary();
  pieceStatusUpdate();
  composeImage();
  glutPostRedisplay();
}


void getImageInfo(string infilename)
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

    in -> close();  // close the file
    delete in;    // free ImageInput
  }
}


/*
get the image pixmap
*/
void readimage(string infilename, unsigned char *inpixmap)
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
    int channels = spec.nchannels;

    unsigned char tmppixmap[xres * yres * channels];
    in -> read_image(TypeDesc::UINT8, tmppixmap);

    // convert input image to RGBA image
    for (int row = 0; row < yres; row++)
    {
      for (int col = 0; col < xres; col++)
      {
        switch (channels)
        {
          case 1:
            inpixmap[(row * xres + col) * 4] = tmppixmap[row * xres + col];
            inpixmap[(row * xres + col) * 4 + 1] = inpixmap[(row * xres + col) * 4];
            inpixmap[(row * xres + col) * 4 + 2] = inpixmap[(row * xres + col) * 4];
            inpixmap[(row * xres + col) * 4 + 3] = 255;
            break;
          case 3:
            for (int k = 0; k < 3; k++)
            {inpixmap[(row * xres + col) * 4 + k] = tmppixmap[(row * xres + col) * 3 + k];}
            inpixmap[(row * xres + col) * 4 + 3] = 255;
            break;
          case 4:
            for (int k = 0; k < 4; k++)
            {inpixmap[(row * xres + col) * 4 + k] = tmppixmap[(row * xres + col) * 4 + k];}
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
        {displaypixmap[(row * xres + col) * 4 + k] = inpixmap[((yres - 1 - row) * xres + col) * 4 + k];}
      }
    }
    for (int i = 0; i < xres * yres * 4; i++) {inpixmap[i] = displaypixmap[i];}

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

    cout << "Write tmp image to image file " << outfilename << endl;
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
// void displayOutput()  {display(outputpixmap, xres_out, yres_out);}
void displayComposed() {display(composedpixmap, xres_out, yres_out);}


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
      if (endflag == false)
      {displayFresh();}
      else  {exit(0);}
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


static void timerFunc(int value)
{
  displayFresh();
  if (endflag == true)  {exit(0);}
  glutTimerFunc(250,timerFunc,0);
}


/*
Main program
*/
int main(int argc, char* argv[])
{
  // random seed initial
  srand(time(NULL));

  // command line parser and calculate transform matrix
  getCmdOptions(argc, argv, inputImage, backImage);
  preprocessing();
  disolvepieces();

  // display input image and output image in seperated windows
  // start up the glut utilities
  glutInit(&argc, argv);
  // create the graphics window, giving width, height, and title text
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);

  // display
  glutInitWindowSize(xres, yres);
  glutCreateWindow("Disintegration Effects");
  // set up the callback routines to be called when glutMainLoop() detects an event
  // glutDisplayFunc(displayOutput);	  // display callback
  glutDisplayFunc(displayComposed);	  // display callback
  if (play_mode == 0)  {glutKeyboardFunc(handleKey);}	  // keyboard callback
  glutMouseFunc(mouseClick);  // mouse callback
  glutReshapeFunc(handleReshape_in); // window resize callback
  if (play_mode == 1)  {glutTimerFunc(FRAME_INTERVAL,timerFunc,0);} // timer callback

  // Routine that loops forever looking for events. It calls the registered
  // callback routine to handle each event that is detected
  glutMainLoop();

  // release memory
  delete [] inputpixmap;
  delete [] outputpixmap;
  delete [] frontpixmap;
  delete [] backpixmap;
  delete [] composedpixmap;

  return 0;
}

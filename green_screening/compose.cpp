/*
Jingcong Zhang
jingcoz@g.clemson.edu
2016-09-26
*/
# include <OpenImageIO/imageio.h>

# include <cstdlib>
# include <iostream>
# include <fstream>
# include <string>

# ifdef __APPLE__
#   pragma clang diagnostic ignored "-Wdeprecated-declarations"
#   include <GLUT/glut.h>
# else
#   include <GL/glut.h>
# endif

using namespace std;
OIIO_NAMESPACE_USING

static string frontimagename; // frontground image file name
static string backimagename;  // background image file name
static string outfilename;  // output image file name
static unsigned char *frontpixmap;  // frontground image pixel map
static unsigned char *backpixmap; // background image pixel map
static unsigned char *composedpixmap; // composed image pixel map
static int xres = 0; // window width
static int yres = 0;  // window height
static int front_w = 0; // frontground image width
static int front_h = 0; // frontground image height
static int backchannels;  // background image channel number


void associatedColor(const unsigned char *pixmap, unsigned char *associatedpixmap, int w, int h)
{
  for (int i = 0; i < w; i++)
  {
    for (int j = 0; j < h; j++)
    {
      float r, g, b, alpha, a;
      r = pixmap[(j * w + i) * 4];
      g = pixmap[(j * w + i) * 4 + 1];
      b = pixmap[(j * w + i) * 4 + 2];
      alpha = pixmap[(j * w + i) * 4 + 3];

      a = float(alpha) / 255;
      associatedpixmap[(j * w + i) * 4] = float(r) * a;
      associatedpixmap[(j * w + i) * 4 + 1] = float(g) * a;
      associatedpixmap[(j * w + i) * 4 + 2] = float(b) * a;
      associatedpixmap[(j * w + i) * 4 + 3] = alpha;
    }
  }
}


/*
over operation
  front value, back value and alpha value on scale 0-255
*/
int over(int front, int back, int alpha)
{
  int composedValue;
  composedValue = float(front) + (1 - (float(alpha) / 255)) * float(back);

  return composedValue;
}


void compose(unsigned char *frontpixmap, unsigned char *backpixmap, int posX, int posY)
{
  composedpixmap = new unsigned char [xres * yres * 4];
  for (int i = 0; i < xres; i++)
  {
    for (int j = 0; j < yres; j++)
    {
      // get associated frontground image pixel value
      float frontR = 0;
      float frontG = 0;
      float frontB = 0;
      float frontA = 0;
      if (i >= posX && i < posX + front_w && j >= posY && j < posY + front_h)
      {
        frontR = frontpixmap[(j * front_w + i) * 4];
        frontG = frontpixmap[(j * front_w + i) * 4 + 1];
        frontB = frontpixmap[(j * front_w + i) * 4 + 2];
        frontA = frontpixmap[(j * front_w + i) * 4 + 3];
      }

      // get associated background image pixel value (background image alpha = 255)
      float backR, backG, backB;
      backR = backpixmap[(j * xres + i) * backchannels];
      backG = backpixmap[(j * xres + i) * backchannels + 1];
      backB = backpixmap[(j * xres + i) * backchannels + 2];
      
      // over operation for each channel value and set alpha value as 255
      composedpixmap[(j * xres + i) * 4] = over(frontR, backR, frontA);
      composedpixmap[(j * xres + i) * 4 + 1] = over(frontG, backG, frontA);
      composedpixmap[(j * xres + i) * 4 + 2] = over(frontB, backB, frontA);
      composedpixmap[(j * xres + i) * 4 + 3] = 255;
    }
  }
}

/*
get the image pixmap
*/
void readimage(string infilename, bool backflag)
{
  // read the input image and store as a pixmap
  ImageInput *in = ImageInput::open(infilename);
  if (!in)
  {
    cerr << "Cannot get the input image for " << infilename << ", error = " << geterror() << endl;
  }
  else
  {
    // get the image size and channels information, allocate space for the image
    const ImageSpec &spec = in -> spec();
    int w, h, channels;   
    w = spec.width;
    h = spec.height;
    channels = spec.nchannels;
    // background image 
    if (backflag)
    {
      xres = w;
      yres = h;
      backchannels = channels;
      backpixmap = new unsigned char [w * h * channels];
      in -> read_image(TypeDesc::UINT8, backpixmap);
    }
    // frontground image
    if (!backflag)
    {
      front_w = w;
      front_h = h;
      if (channels < 4)
      {
        cout << "Frontimage should have 4 channels." << endl;
        exit(0);
      }
      unsigned char pixmap[w * h * channels];
      frontpixmap = new unsigned char [w * h * channels];
      in -> read_image(TypeDesc::UINT8, pixmap);
      associatedColor(pixmap, frontpixmap, w, h);
    }

    in -> close();  // close the file

    delete in;    // free ImageInput
  }
}


/*
write out the mask image
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
    ImageSpec spec (xres, yres, 4, TypeDesc::UINT8);
    out -> open(outfilename, spec);
    // write the entire image
    out -> write_image(TypeDesc::UINT8, composedpixmap);
    cout << "Write the image pixmap to image file " << outfilename << endl;
    // close the file and free the ImageOutput I created
    out -> close();

    delete out;
  }
}


void display()
{    
  // modify the pixmap: upside down the image and add A channel value for the image
  unsigned char displaypixmap[xres * yres * 4];
  int i, j, k; 
  for (i = 0; i < xres; i++)
  {
    for (j = 0; j < yres; j++)
    {   
      for (k = 0; k < 4; k++)
      {
        displaypixmap[(j * xres + i) * 4 + k] = composedpixmap[((yres - 1 - j) * xres + i) * 4 + k];
      }
    }
  }

  // display the pixmap
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glRasterPos2i(0, 0);
  // glDrawPixels writes a block of pixels to the framebuffer.
  glDrawPixels(xres, yres, GL_RGBA, GL_UNSIGNED_BYTE, displaypixmap);
  glFlush();
}


/*
Keyboard Callback Routine: 'w' or 'W' write the pixmap to an image file, 'q', 'Q' or ESC quit
This routine is called every time a key is pressed on the keyboard
*/
void handleKey(unsigned char key, int x, int y)
{
  switch(key)
  {
    case 'w':
    case 'W':
      // writeimage();
      break;
      
    case 'q':		// q - quit
    case 'Q':
    case 27:		// esc - quit
      exit(0);
      
    default:		// not a valid key -- just ignore it
      return;
  }
}


/*
Reshape Callback Routine: sets up the viewport and drawing coordinates
*/
void handleReshape(int w, int h)
{
  /*
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
  */
  
  // define the drawing coordinate system on the viewport
  // to be measured in pixels
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, w, 0, h);
  glMatrixMode(GL_MODELVIEW);
}


/*
Main program
*/
int main(int argc, char* argv[])
{
  // command line: get frontground image and background image
  // usage: compose <frontimagename> <backimagename> (optional)<outputfilename>
  if (argc >= 3)  // one argument of image file name
  {
    cout << "Frontground image file name: " << argv[1] << endl;
    cout << "Background image file name: " << argv[2] << endl;
    frontimagename = argv[1];
    backimagename = argv[2];
    if (argc > 3)
      {
        cout << "Output image file name: " << argv[3] << endl;
        outfilename = argv[3];
      }
  }
  else
  {
    cout << "[Usage] compose <frontimagename> <backimagename> (optional)<outputfilename>" << endl;
    return 0;
  }

  // read input image
  cout << "Read frontground image..." << endl;
  readimage(frontimagename, 0);
  cout << "Read background image..." << endl;
  readimage(backimagename, 1);
  cout << "Compose images..." << endl;
  compose(frontpixmap, backpixmap, 0, 0);
  if (argc > 3)
  {
    cout << "Write composed associated image..." << endl;
    writeimage(outfilename);
  }
  
  // window size when no argument of image file
  int w = xres;
  int h = yres;
  
  // start up the glut utilities
  glutInit(&argc, argv);
  
  // create the graphics window, giving width, height, and title text
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
  glutInitWindowSize(w, h);
  glutCreateWindow("Green Screening Image");
  
  // set up the callback routines to be called when glutMainLoop() detects an event
  glutDisplayFunc(display);	  // display callback
  // glutKeyboardFunc(handleKey);	  // keyboard callback
  glutReshapeFunc(handleReshape); // window resize callback
  
  // Routine that loops forever looking for events. It calls the registered
  // callback routine to handle each event that is detected
  glutMainLoop();

  delete frontpixmap;
  delete backpixmap;

  return 0;
}


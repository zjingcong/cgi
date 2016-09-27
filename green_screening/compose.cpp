/*
OpenGL and GLUT program to compose the frontground png image with the background image.
Background image's alpha channel should be 1 and should be larger than frontground image.

Usage: compose <front_image_file> <back_image_file> (optional)<output_file_name>
    The program will compose the frontground image and background image, 
    display the associated composed image and optionally write out the associated composed image from the pixel map.
    Frontground image default position is in the middle. 
    User can simply modify the frontground image and write out the current window using key response.
Key Response:
    r or R - move the frontground image to right
    l or L - move the frontground image to left
    w or W - write the current window to an image file
    q or Q or ESC - exit the program

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
static int posX;  // frontground image X position
static int posY;  // frontground image Y position


/*
generate associated color image
*/
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


/*
composition
  convert frontground image to associated color image (no need to do so for background image due to 255 alpha value)
  and do the over operation between associated frontground image and background image
*/
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
        int x, y;
        x = i - posX;
        y = j - posY;
        frontR = frontpixmap[(y * front_w + x) * 4];
        frontG = frontpixmap[(y * front_w + x) * 4 + 1];
        frontB = frontpixmap[(y * front_w + x) * 4 + 2];
        frontA = frontpixmap[(y * front_w + x) * 4 + 3];
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
    exit(0);
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


/*
display composed associated color image
*/
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
Routine to get image from OpenGL framebuffer and then write the associated composed image to an image file
*/
void writeglimage()
{   
  // get the output file name
  string outfile;
  cout << "Enter output image filename: ";
  cin >> outfile;
  
  int outchannels = 4;
  if ((outfile.substr(outfile.find_last_of(".") + 1, outfile.length() - 1) == "ppm"))  {outchannels = 3;}

  // get the current image from the OpenGL framebuffer and store in pixmap
  int w = glutGet(GLUT_WINDOW_WIDTH);
  int h = glutGet(GLUT_WINDOW_HEIGHT);
  unsigned char glpixmap[w * h * outchannels];
  if (outchannels == 4) {glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, glpixmap);}
  else {glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, glpixmap);}

  // set the image upside down
  unsigned char writepixmap[w * h * outchannels];
  int i, j, k; 
  for (i = 0; i < w; i++)
  {
    for (j = 0; j < h; j++)
    {   
      for (k = 0; k < outchannels; k++)
      {
        writepixmap[(j * w + i) * outchannels + k] = glpixmap[((h - 1 - j) * w + i) * outchannels + k];
      }
    }
  }

  // create the subclass instance of ImageOutput which can write the right kind of file format
  ImageOutput *out = ImageOutput::create(outfile);
  if (!out)
  {
    cerr << "Could not create output image for " << outfilename << ", error = " << geterror() << endl;
  }
  else
  {   
    // open and prepare the image file
    cout << "outchannels: " << outchannels << endl;
    ImageSpec spec (w, h, outchannels, TypeDesc::UINT8);
    out -> open(outfile, spec);
    // write the entire image
    out -> write_image(TypeDesc::UINT8, writepixmap);
    cout << "Write the image pixmap to image file " << outfile << endl;
    // close the file and free the ImageOutput I created
    out -> close();
    
    delete out;
  }
}


/*
Keyboard Callback Routine: 'w' or 'W' write current window to an image file, 'r', 'R' move frontground image to right, 
                           'l' or 'L' move frontground image to left, 'q', 'Q' or ESC quit
This routine is called every time a key is pressed on the keyboard
*/
void handleKey(unsigned char key, int x, int y)
{
  switch(key)
  {
    // write the current window to an image file
    case 'w':
    case 'W':
      writeglimage();
      break;
    
    // frontground image move left
    case 'l':
    case 'L':
      posX = (posX - 150) % (xres - front_w);
      if (posX <= 0)  {posX = xres - front_w - posX;}
      compose(frontpixmap, backpixmap, posX, posY);
      glutPostRedisplay();
      break;
    
    // frontground image move right
    case 'r':
    case 'R':
      posX = (posX + 150) % (xres - front_w);
      compose(frontpixmap, backpixmap, posX, posY);
      glutPostRedisplay();
      break;
      
    // quit the program
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

  // set frontground image position as middle
  posX = float(xres - front_w) / 2;  
  posY = yres - front_h;
  // compose frountground image with background image
  cout << "Compose images..." << endl;
  compose(frontpixmap, backpixmap, posX, posY);

  // write composed associated image
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
  glutCreateWindow("Composed Dr.House Image");
  
  // set up the callback routines to be called when glutMainLoop() detects an event
  glutDisplayFunc(display);	  // display callback
  glutKeyboardFunc(handleKey);	  // keyboard callback
  glutReshapeFunc(handleReshape); // window resize callback
  
  // Routine that loops forever looking for events. It calls the registered
  // callback routine to handle each event that is detected
  glutMainLoop();

  delete frontpixmap;
  delete backpixmap;
  delete composedpixmap;

  return 0;
}


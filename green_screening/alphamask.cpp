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

static string infilename; // input image name
static string outfilename;  // output image name
static int xres;  // input image width
static int yres;  // input image height
static unsigned char *pixmap;  // input image pixel map
static unsigned char *outpixmap;  // output image pixel map

# define maximum(x, y, z) ((x) > (y)? ((x) > (z)? (x) : (z)) : ((y) > (z)? (y) : (z)))
# define minimum(x, y, z) ((x) < (y)? ((x) < (z)? (x) : (z)) : ((y) < (z)? (y) : (z)))


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
  }
  else
  {
    // get the image size and channels information, allocate space for the image
    const ImageSpec &spec = in -> spec();
    int channels;
    xres = spec.width;
    yres = spec.height;
    channels = spec.nchannels;
    
    pixmap = new unsigned char [xres * yres * channels];
    in -> read_image(TypeDesc::UINT8, pixmap);
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
    out -> write_image(TypeDesc::UINT8, outpixmap);
    cout << "Write the image pixmap to image file " << outfilename << endl;
    // close the file and free the ImageOutput I created
    out -> close();

    delete out;
  }
}


void alphamask()
{
  // get thresholds
  double th_hl_1, th_hl_2, th_s_1, th_s_2, th_v_1, th_v_2, th_hh_1, th_hh_2;
  get_thresholds(th_hl_1, th_hl_2, th_s_1, th_s_2, th_v_1, th_v_2, th_hh_1, th_hh_2);

  // convert rgb color to hsv color
  outpixmap = new unsigned char [xres * yres * 4];
  for (int i = 0; i < xres; i++)
  {
    for (int j = 0; j < yres; j++)
    {
      int r, g, b;
      double h, s, v;
      unsigned char alpha;
      r = pixmap[(j * xres + i) * 3];
      g = pixmap[(j * xres + i) * 3 + 1];
      b = pixmap[(j * xres + i) * 3 + 2];
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
          double mid;
          mid = (th_hl_2 - th_hl_1) / 2 + th_hl_1;
          if (h >= mid) {alpha = (h / (th_hh_2 - mid)) * 255;}
          else {alpha = (h / (mid - th_hh_1)) * 255;}
        }
      }

      outpixmap[(j * xres + i) * 4] = r;
      outpixmap[(j * xres + i) * 4 + 1] = g;
      outpixmap[(j * xres + i) * 4 + 2] = b;
      outpixmap[(j * xres + i) * 4 + 3] = alpha;
    }
  }
}


/*
Main program
*/
int main(int argc, char* argv[])
{
  // command line: get inputfilename and outputfilename
  // usage: alphamask <inputfilename> <outputfilename>
  if (argc == 3)  // one argument of image file name
  {
    cout << "Input image file name: " << argv[1] << endl;
    cout << "Output image file name: " << argv[2] << endl;
    infilename = argv[1];
    outfilename = argv[2];
  }
  else
  {
    cout << "[Usage] alphamask <inputfilename> <outputfilename>" << endl;
    cout << "Out put image file type should be PNG file." << endl;

    return 0;
  }

  // read input image
  cout << "Read Image..." << endl;
  readimage(infilename);
  // generate alpha channel mask
  cout << "Processing..." << endl;
  alphamask();
  // write out the image
  cout << "Write Image..." << endl;
  writeimage(outfilename);
  
  /*
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
  glutKeyboardFunc(handleKey);	  // keyboard callback
  glutReshapeFunc(handleReshape); // window resize callback
  
  // Routine that loops forever looking for events. It calls the registered
  // callback routine to handle each event that is detected
  glutMainLoop();
  */

  delete pixmap;
  delete outpixmap;

  return 0;
}


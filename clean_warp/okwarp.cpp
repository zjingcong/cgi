/*
OpenGL and GLUT program to warp an input image using inverse mapping from two different warp functions, 
with fixing the minification using adaptive supersampling and magnification problems using bilinear interpolation, 
display the input image and output image, and then optionally write out to an image file.
Users can select warp mode to decide methods taken to clean the warp and warp functions by command lines.

Usage: 
  okwarp input_image_name [output_image_name]
    -m mode selection: 0-4
    -w warp function selection: 0-1
  mode selection:
    1: general warp
    2: warp with clean method supersampling for minification
    3: warp with clean method adaptive supersampling for minification
    4: warp with clean method bilinear interpolation for magnification
    0: all clean warp (adaptive supersampling + bilinear interpolation)
    default mode: 0
  warp function selection:
    0: dr.house's warp function
    1: my warp function

Mouse Response:
  click the window to quit the program

Keyboard Response:
  Press q, Q or exit to quit the program

Jingcong Zhang
jingcoz@g.clemson.edu
2016-11-10
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

# ifndef PI
# define PI 3.1415926536
# endif

# define SAMPLING_TH 65 // predefined adaptive supersampling threshold, 
                        // decided from the difference between each samples and their average when testing

using namespace std;
OIIO_NAMESPACE_USING


static unsigned char *inputpixmap;  // input image pixels pixmap
static unsigned char *outputpixmap; // output image pixels pixmap
static int xres, yres;  // input image size: width, height
static int xres_out, yres_out;  // output image size: width, height
int mode;
int warp_id;  // warp function 0: dr. house's okwarp function, 1: my warp function


/*
  Routine to inverse map (x, y) output image spatial coordinates
  into (u, v) input image spatial coordinates

  Call routine with (x, y) spatial coordinates in the output
  image. Returns (u, v) spatial coordinates in the input image,
  after applying the inverse map. Note: (u, v) and (x, y) are not 
  rounded to integers, since they are true spatial coordinates.
 
  inwidth and inheight are the input image dimensions
  outwidth and outheight are the output image dimensions
*/
void inv_map(float x, float y, float &u, float &v, int inwidth, int inheight, int outwidth, int outheight)
{  
  x /= outwidth;		// normalize (x, y) to (0...1, 0...1)
  y /= outheight;

  switch (warp_id)
  {
    // dr.house's warp function
    case 0:
      u = sqrt(x);			        // inverse in x direction is sqrt
      v = 0.5 * (1 + sin(y * PI));  // inverse in y direction is offset sine
      break;
    // my warp function
    case 1:
      u = pow(x, 0.7);
      v = pow((sin(M_PI * y / 2)), 2.0);
      break;
    default:
      cout << "please select warp function between 0-1." << endl;
      exit(0);
  }

  u *= inwidth;			// scale normalized (u, v) to pixel coords
  v *= inheight;

  u = (u == inwidth) ? (u - 0.0001) : u;
  v = (v == inheight) ? (v - 0.0001) : v;
}


/*
scale factor calculator
  scale_factor > 1: minification
  scale_factor < 1: magnification
*/
void scale_factor(double x, double y, double &scale_factor_x, double &scale_factor_y)
{
  // calculate four corners of one output pixel value in the input image
  // in order to calculate scale factor in each direction (x direction and y direction)
  float u0, v0, u1, v1, u2, v2, u3, v3;
  inv_map(x - 0.5, y - 0.5, u0, v0, xres, yres, xres_out, yres_out);
  inv_map(x + 0.5, y - 0.5, u1, v1, xres, yres, xres_out, yres_out);
  inv_map(x - 0.5, y + 0.5, u2, v2, xres, yres, xres_out, yres_out);
  inv_map(x + 0.5, y + 0.5, u3, v3, xres, yres, xres_out, yres_out);
  
  scale_factor_x = ((u1 - u0) + (u3 - u2)) / 2;
  scale_factor_y = ((v2 - v0) + (v3 - v1)) / 2;
}


/*
magnification fix: 
bilinear interpolation
  c_output = (1 - s) * (1 - t) * c0 + s * (1 - t) * c1 + (1 - s) * t * c2 + s * t * c3
*/
unsigned char bilinear_interpolation(double u, double v, int row_out, int col_out, int channel, unsigned char pixmap[])
{
  // calculate the positions of four points to do the bilinear interpolation
  double u0, v0, u1, v1, u2, v2, u3, v3, s, t;
  u0 = (u >= (floor(u) + 0.5)) ? (floor(u) + 0.5) : (floor(u) - 0.5);
  v0 = (v >= (floor(v) + 0.5)) ? (floor(v) + 0.5) : (floor(v) - 0.5);
  s = u - u0;
  t = v - v0;

  // boundary area
  if ((v >= yres - 0.5) || (v <= 0.5)) 
  {
    t = 0;
    v0 = (v >= yres - 0.5) ? (yres - 0.5) : (0.5);
  }
  if ((u >= xres - 0.5) || (u <= 0.5))
  {
    s = 0;
    u0 = (u >= xres - 0.5) ? (xres - 0.5) : (0.5);
  }

  u1 = u0 + 1;
  v1 = v0;
  u2 = u0;
  v2 = v0 + 1;
  u3 = u0 + 1;
  v3 = v0 + 1;

  // get the color of four points
  double c0, c1, c2, c3;
  unsigned char c_out;
  c0 = pixmap[(int(floor(v0)) * xres + int(floor(u0))) * 4 + channel];
  c1 = pixmap[(int(floor(v1)) * xres + int(floor(u1))) * 4 + channel];
  c2 = pixmap[(int(floor(v2)) * xres + int(floor(u2))) * 4 + channel];
  c3 = pixmap[(int(floor(v3)) * xres + int(floor(u3))) * 4 + channel];
  c_out = (1 - s) * (1 - t) * c0 + s * (1 - t) * c1 + (1 - s) * t * c2 + s * t * c3;
  
  return c_out;
}


/*
minification fix: 
supersampling
*/
void supersampling(int row_in, int col_in, unsigned char super_inputpixmap[])
{
  double weight_list[] = {1.0, 2.0, 1.0, 
                          2.0, 8.0, 2.0, 
                          1.0, 2.0, 1.0};
  // get the multisamples
  int index_list [9] = {0};
  int index = 0;
  for (int i = -1; i <= 1; ++i)
  {
    for (int j = -1; j <= 1; ++j)
    {
      if ((row_in + i) < yres && (row_in + i) >= 0 && (col_in + j) < xres && (col_in + j) >= 0)
        {index_list[index] = (row_in + i) * xres + (col_in + j);}
      // exclude samples located input image's outside
      else {index_list[index] = -1;}
      index++;
    }
  }
  // calculate the area average with weights predefined in weight_list
  for (int channel = 0; channel < 4; channel++)
  {
    double weight_count = 0;
    double sum = 0;
    unsigned char c_out;
    for (int i = 0; i < 9; ++i)
    {
      if (index_list[i] != -1)
      {
        sum += weight_list[i] * inputpixmap[index_list[i] * 4 + channel];
        weight_count += weight_list[i];
      }
    }
    c_out = sum / weight_count;
    super_inputpixmap[(row_in * xres + col_in) * 4 + channel] = c_out;
  }
}


/*
minification fix: 
adaptive supersampling
*/
void ad_supersampling(int row_in, int col_in, unsigned char adsuper_inputpixmap[])
{
  double weight_list[] = {1.0, 2.0, 1.0, 
                          2.0, 8.0, 2.0, 
                          1.0, 2.0, 1.0};
  int index_list [9] = {0};
  int index = 0;
  for (int i = -1; i <= 1; ++i)
  {
    for (int j = -1; j <= 1; ++j)
    {
      if ((row_in + i) < yres && (row_in + i) >= 0 && (col_in + j) < xres && (col_in + j) >= 0)
        {index_list[index] = (row_in + i) * xres + (col_in + j);}
      // exclude samples located input image's outside
      else {index_list[index] = -1;}
      index++;
    }
  }
  
  for (int channel = 0; channel < 4; channel++)
  {
    // calculate samples average value
    double sample_sum = 0;
    double sample_count = 0;
    for (int i = 0; i < 9; i++)
    {
      if (index_list[i] != -1)
      {
        sample_sum += inputpixmap[index_list[i] * 4 + channel];
        sample_count++;
      }
    }
    double sample_avg = sample_sum / sample_count;
    // exclude extreme pixel which have a larger difference from average value than predefined threshold
    for (int i = 0; i < 9; i++)
    {
      if (index_list[i] != -1)
      {
        double diff = abs(inputpixmap[index_list[i] * 4 + channel] - sample_avg);
        if (diff > SAMPLING_TH) {index_list[i] = -1;}
      }
    }
  }

  // calculate the area average with weights
  for (int channel = 0; channel < 4; channel++)
  {
    double weight_count = 0;
    double sum = 0;
    unsigned char c_out;
    for (int i = 0; i < 9; ++i)
    {
      if (index_list[i] != -1)
      {
        sum += weight_list[i] * inputpixmap[index_list[i] * 4 + channel];
        weight_count += weight_list[i];
      }
    }
    c_out = sum / weight_count;
    adsuper_inputpixmap[(row_in * xres + col_in) * 4 + channel] = c_out;
  }
}


/*
warp image
*/
void warpimage()
{
  xres_out = xres;
  yres_out = yres;
  cout << "Output image size: " << xres_out << "x" << yres_out << endl;
  // allocate memory for output pixmap: output image is 4 channels
  outputpixmap = new unsigned char [xres_out * yres_out * 4];
  // fill the output image with a clear transparent color(0, 0, 0, 0)
  for (int i = 0; i < xres_out * yres_out * 4; i++) {outputpixmap[i] = 0;}

  // supersampling & adaptive supersampling
  unsigned char super_inputpixmap[xres * yres * 4];
  unsigned char adsuper_inputpixmap[xres * yres * 4];
  for (int row_in = 0; row_in < yres; row_in++)
  {
    for (int col_in = 0; col_in < xres; col_in++)
      {
        // supersampling
        supersampling(row_in, col_in, super_inputpixmap);
        // adaptive supersampling
        ad_supersampling(row_in, col_in, adsuper_inputpixmap);
      }
  }

  // inverse map
  float x, y, u, v;
  for (int row_out = 0; row_out < yres_out; row_out++)  // output row
  {
    for (int col_out = 0; col_out < xres_out; col_out++)  // output col
    {
      double scale_factor_x = 1.0;
      double scale_factor_y = 1.0;

      x = float(col_out) + 0.5;
      y = float(row_out) + 0.5;      
      // inverse mapping functions
      inv_map(x, y, u, v, xres, yres, xres_out, yres_out);
      // calculate scale factor
      scale_factor(x, y, scale_factor_x, scale_factor_y);

      if (u < xres && v < yres && u >= 0 && v >= 0)
      {
        int row_in, col_in;
        row_in = floor(v);
        col_in = floor(u);

        for (int k = 0; k < 4; k++)
        {
          switch (mode)
          {
            // general warp
            case 1:
              outputpixmap[(row_out * xres_out + col_out) * 4 + k] = inputpixmap[(row_in * xres + col_in) * 4 + k];
              break;

            // supersampling for minification
            case 2:
              if (scale_factor_x > 1 || scale_factor_y > 1)
                {outputpixmap[(row_out * xres_out + col_out) * 4 + k] = super_inputpixmap[(row_in * xres + col_in) * 4 + k];}
              else {outputpixmap[(row_out * xres_out + col_out) * 4 + k] = inputpixmap[(row_in * xres + col_in) * 4 + k];}
              break;

            // adaptive supersampling for minification
            case 3:
              if (scale_factor_x > 1 || scale_factor_y > 1)
                {outputpixmap[(row_out * xres_out + col_out) * 4 + k] = adsuper_inputpixmap[(row_in * xres + col_in) * 4 + k];}
              else {outputpixmap[(row_out * xres_out + col_out) * 4 + k] = inputpixmap[(row_in * xres + col_in) * 4 + k];}
              break;

            // bilinear interpolation for magnification
            case 4:
              if (scale_factor_x < 1 || scale_factor_y < 1)
                {outputpixmap[(row_out * xres_out + col_out) * 4 + k] 
                = bilinear_interpolation(u, v, row_out, col_out, k, inputpixmap);}
              else  {outputpixmap[(row_out * xres_out + col_out) * 4 + k] = inputpixmap[(row_in * xres + col_in) * 4 + k];}
              break;

            // all clean warp            
            case 0:
              // magnification
              if (scale_factor_x < 1 || scale_factor_y < 1)
              {
                if (scale_factor_x < 1 && scale_factor_y < 1)
                  {outputpixmap[(row_out * xres_out + col_out) * 4 + k] 
                    = bilinear_interpolation(u, v, row_out, col_out, k, inputpixmap);}
                // pixel is minified in one direction and magnified in the other direction
                else
                  {outputpixmap[(row_out * xres_out + col_out) * 4 + k] 
                    = bilinear_interpolation(u, v, row_out, col_out, k, adsuper_inputpixmap);}
              }
              else if (scale_factor_x == 1 && scale_factor_y == 1)
                {outputpixmap[(row_out * xres_out + col_out) * 4 + k] = inputpixmap[(row_in * xres + col_in) * 4 + k];}
              // minification
              else
                {outputpixmap[(row_out * xres_out + col_out) * 4 + k] = adsuper_inputpixmap[(row_in * xres + col_in) * 4 + k];}
              break;

            default:
              cout << "please select mode between 0-4." << endl;
              exit(0);
          }
        }
      }
    }
  }
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
command line options parser
  warp input_image_name [output_image_name]
    -m mode selection: 0-4
    -w warp function selection: 0-1
*/
char **getIter(char** begin, char** end, const std::string& option) {return find(begin, end, option);}
void getCmdOptions(int argc, char* argv[], string &inputImage, string &outputImage, int &mode)
{
  string mode_list[10];
  mode_list[1] = "general warp";
  mode_list[2] = "warp with clean method supersampling for minification";
  mode_list[3] = "warp with clean method adaptive supersampling for minification";
  mode_list[4] = "warp with clean method bilinear interpolation for magnification";
  mode_list[0] = "all clean warp (adaptive supersampling + bilinear interpolation)";
  if (argc >= 2)
  {
    mode = 0; // default mode
    warp_id = 0;  // default warp function

    // get input image name
    inputImage = argv[1];
    if (argc >= 3)
    {
      // get output image name
      string tmp;
      tmp = argv[2];
      if (tmp != "-m" && tmp != "-w")  {outputImage = argv[2];}
      // mode selection
      char **iter = getIter(argv, argv + argc, "-m");
      if (iter != argv + argc)  {if (++iter != argv + argc)  {mode = atoi(iter[0]);}}
      if (mode > 4 || mode < 0)
      {
        cout << "please select mode between 0-4." << endl;
        exit(0);
      }
      cout << "warp mode " << mode << ": " << mode_list[mode] << endl;
      // warp function selection
      iter = getIter(argv, argv + argc, "-w");
      if (iter != argv + argc)  {if (++iter != argv + argc)  {warp_id = atoi(iter[0]);}}
      cout << "warp function: " << warp_id << endl;
    }
  }
  else
  {
    cout << "[HELP]" << endl;
    cout << "okwarp input_image_name [output_image_name]" << endl;
    cout << "  -m mode selection" << endl;
    cout << "    1: " << mode_list[1] << "\n"
         << "    2: " << mode_list[2] << "\n"
         << "    3: " << mode_list[3] << "\n"
         << "    4: " << mode_list[4] << "\n"
         << "    0: " << mode_list[0] << "\n"
         << "    default mode: mode 0" << endl;
    cout << "  -w warp function selection" << endl;
    cout << "    0: dr. house's okwarp function\n"
         << "    1: my warp function\n"
         << "    default warp function: 0"
         << endl;
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
  getCmdOptions(argc, argv, inputImage, outputImage, mode);

  // read input image
  readimage(inputImage);
  // warp the original image
  warpimage();
  // write out to an output image file
  if (outputImage != "") {writeimage(outputImage);}
  
  // display input image and output image in seperated windows
  // start up the glut utilities
  glutInit(&argc, argv);
  
  // create the graphics window, giving width, height, and title text
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);

  // first window: input image
  glutInitWindowSize(xres, yres);
  glutCreateWindow("Input Image");  
  // set up the callback routines to be called when glutMainLoop() detects an event
  glutDisplayFunc(displayInput);	  // display callback
  glutMouseFunc(mouseClick);  // mouse callback
  glutReshapeFunc(handleReshape_in); // window resize callback

  // second window: output image
  glutInitWindowSize(xres_out, yres_out);
  glutCreateWindow("Warped Image");
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


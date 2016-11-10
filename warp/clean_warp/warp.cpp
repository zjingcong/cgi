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

using namespace std;
OIIO_NAMESPACE_USING


static unsigned char *inputpixmap;  // input image pixels pixmap
static unsigned char *outputpixmap; // output image pixels pixmap
static int xres, yres;  // input image size: width, height
static int xres_out, yres_out;  // output image size: width, height
int mode; // mode 1: general warp, mode 2: supersampling, mode 3: supersampling + bilinear interpolation


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

  u = sqrt(x);			        // inverse in x direction is sqrt
  v = 0.5 * (1 + sin(y * PI));  // inverse in y direction is offset sine

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
                          2.0, 4.0, 2.0, 
                          1.0, 2.0, 1.0};
  int index_list [9] = {0};
  int index = 0;
  // cout << "row_in = " << row_in << " col_in = " << col_in << endl;
  for (int i = -1; i <= 1; ++i)
  {
    for (int j = -1; j <= 1; ++j)
    {
      if ((row_in + i) < yres && (row_in + i) >= 0 && (col_in + j) < xres && (col_in + j) >= 0)
        {index_list[index] = (row_in + i) * xres + (col_in + j);}
      else {index_list[index] = -1;}
      index++;
    }
  }
  // for (int i = 0; i < 9; ++i) {cout << index_list[i] << " ";} cout << endl;
  for (int channel = 0; channel < 4; channel++)
  {
    double weight_count = 0;
    double sum = 0;
    unsigned char c_out;
    // cout << channel << endl;
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

  // supersampling
  unsigned char super_inputpixmap[xres * yres * 4];
  for (int row_in = 0; row_in < yres; row_in++)
  {
    for (int col_in = 0; col_in < xres; col_in++)
      {supersampling(row_in, col_in, super_inputpixmap);}
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
            case 0:
              outputpixmap[(row_out * xres_out + col_out) * 4 + k] = inputpixmap[(row_in * xres + col_in) * 4 + k];
              break;
            case 1:
              outputpixmap[(row_out * xres_out + col_out) * 4 + k] = super_inputpixmap[(row_in * xres + col_in) * 4 + k];
              break;
            case 2:
              // magnification
              if (scale_factor_x < 1 || scale_factor_y < 1)
                {outputpixmap[(row_out * xres_out + col_out) * 4 + k] 
                = bilinear_interpolation(u, v, row_out, col_out, k, inputpixmap);}
              else  {outputpixmap[(row_out * xres_out + col_out) * 4 + k] = inputpixmap[(row_in * xres + col_in) * 4 + k];}
              break;
            case 3:
              // magnification
              if (scale_factor_x < 1 || scale_factor_y < 1)
                {outputpixmap[(row_out * xres_out + col_out) * 4 + k] 
                = bilinear_interpolation(u, v, row_out, col_out, k, super_inputpixmap);}
              else  {outputpixmap[(row_out * xres_out + col_out) * 4 + k] = super_inputpixmap[(row_in * xres + col_in) * 4 + k];}
              break;
            default:
              return;
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
  warp input_image_name [output_image_name] -m [mode]
    mode 1: general warp, mode 2: supersampling, mode 3: supersampling + bilinear interpolation
*/
char **getIter(char** begin, char** end, const std::string& option) {return find(begin, end, option);}
void getCmdOptions(int argc, char* argv[], string &inputImage, string &outputImage, int &mode)
{
  string mode_list[4];
  mode_list[0] = "general warp";
  mode_list[1] = "warp with clean method supersampling";
  mode_list[2] = "warp with clean method bilinear interpolation";
  mode_list[3] = "all clean warp (supersampling + bilinear interpolation)";
  if (argc >= 2)
  {
    inputImage = argv[1];
    char **iter = getIter(argv, argv + argc, "-m");
    if (iter != argv + argc)
    {
      if (++iter != argv + argc)  {mode = atoi(iter[0]);}
      if (argc >= 5)  {outputImage = argv[2];}
    }
    else
    {
      mode = 3; // default mode: all clean warp (supersampling + bilinear interpolation)
      if (argc >= 3)  {outputImage = argv[2];}
    }
    cout << "warp mode " << mode << ": " << mode_list[mode] << endl;
  }
  else
  {
    cout << "[HELP]" << endl;
    cout << "warp input_image_name [output_image_name] -m [mode]" << endl;
    cout << " - mode 0: general warp\n"
         << " - mode 1: warp with clean method supersampling\n"
         << " - mode 2: warp with clean method bilinear interpolation\n"
         << " - mode 3: default mode, all clean warp (supersampling + bilinear interpolation)" 
         << endl;
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


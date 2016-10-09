/*
OpenGL and GLUT program to wrap an input image, display original ans wrapped image, and optionally write out the image to an image file.
The program provide two types of wrap operation:
  mode 1 - stretch image: magnify the right side and minify the left side
  mode 2 - twirl image: increase the wrap parameter to enhance image twirling
The default mode is twirl image with wrap parameter 2.
The wrapped image size keep the same with the original one.

Usage: 
wrap input_image_name [output_image_name] [mode] [wrap_parameter]

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
static int xres, yres;  // input image size: width, height
static int xres_out, yres_out;  // output image size: width, height


/*
wrap image
  mode 1 - stretch image
  mode 2 - twirl image
*/
void wrapimage(int mode, double parameter)
{
  // xres_out = xres * pow(2.0, 0.5);
  // yres_out = yres * pow(2.0, 0.5);
  xres_out = xres;
  yres_out = yres;  
  
  // allocate memory for output pixmap: output image is 4 channels
  outputpixmap = new unsigned char [xres_out * yres_out * 4];
  // fill the output image with a clear transparent color(0, 0, 0, 0)
  for (int i = 0; i < xres_out * yres_out * 4; i++) {outputpixmap[i] = 0;} 
  
  // twirl image transformation parameters
  double twirl_f = parameter;

  // get the min, max of x, y coordinates of the original image 
  // to find the range of wrapped image
  // calculate the entire image, not only the 4 corners of the image
  // because the max/min value of x, y coordinates may not on the corners
  double scale_factor_x, scale_factor_y;
  double x_max = 0;
  double y_max = 0;
  double x_min = 1;
  double y_min = 1;
  for (int row = 0; row < yres; row++)
  {
    for (int col = 0; col < xres; col++)
    {
      double x, y, uu, vv, r, theta;
      // coordinate normalization
      double u = (float(col) + 0.5) / float(xres);
      double v = (float(row) + 0.5) / float(yres);
      
      switch (mode)
      {
        case 1:
          break;
        case 2:
          // wrap function 2: twirl image
          uu = (u - 0.5) * 2;
          vv = (v - 0.5) * 2;
          r = pow((pow(uu, 2.0) + pow(vv, 2.0)), 0.5);
          theta = atan2(vv, uu);
          x = r * cos(theta - twirl_f * r) / 2 + 0.5;
          y = r * sin(theta - twirl_f * r) / 2 + 0.5;
          break;
        default:
          return;
      }

      x_max = (x > x_max) ? x : x_max;
      y_max = (y > y_max) ? y : y_max;
      x_min = (x < x_min) ? x : x_min;
      y_min = (y < y_min) ? y : y_min;
    }
  }
  scale_factor_x = x_max - x_min;
  scale_factor_y = y_max - y_min;

  // inverse map
  double x, y, u, v;
  for (int row_out = 0; row_out < yres_out; row_out++)  // output row
  {
    for (int col_out = 0; col_out < xres_out; col_out++)  // output col
    {
      // coordinate normalization
      x = (float(col_out) + 0.5) / float(xres_out);
      y = (float(row_out) + 0.5) / float(yres_out);
      
      double xx, yy, r, a;

      switch (mode)
      {
        case 1:
          // wrap fuction 1
          u = pow(x, 0.25);
          v = pow((sin(M_PI * y / 2)), 2.0);
          break;
        case 2:
          // wrap fuction 2: twirl image
          xx = ((x * scale_factor_x + x_min) - 0.5) * 2;
          yy = ((y * scale_factor_y + y_min) - 0.5) * 2;

          r = pow((pow(xx, 2.0) + pow(yy, 2.0)), 0.5);
          a = atan2(yy, xx);
          u = (r * cos(a + twirl_f * r) / 2) + 0.5;
          v = (r * sin(a + twirl_f * r) / 2) + 0.5;
          break;
        default:
          return;      
      }

      if (u <= 1 && v <= 1 && u >= 0 && v >= 0)
      {
        int row_in, col_in;
        row_in = floor(v * yres); // yres = H_input
        col_in = floor(u * xres); // xres = W_input
        
        for (int k = 0; k < 4; k++)
        {outputpixmap[(row_out * xres_out + col_out) * 4 + k] = inputpixmap[(row_in * xres + col_in) * 4 + k];}
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
  if (w < xres || h < yres)
  {
    float xfactor = w / float(xres);
    float yfactor = h / float(yres);
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
  wrap input_image_name [output_image_name](optional)
*/
void getCmdOptions(int argc, char* argv[], string &inputImage, string &outputImage, int &mode, double &parameter)
{
  if (argc >= 2)
  {
    inputImage = argv[1];
    if (argc > 2)
    {
      string tmp = argv[2];
      if (tmp == "1" || tmp == "2") {mode = atoi(argv[2]);  if (argc > 3) {parameter = atof(argv[3]);}}
      else
      {
        outputImage = argv[2];
        if (argc > 3)
        {
          tmp = argv[3];
          if (tmp == "1" || tmp == "2") {mode = atoi(argv[3]);  if (argc > 4) {parameter = atof(argv[4]);}}
        }
      }
    }
  }
  // print help message
  else  
  {
    cout << "[HELP]" << endl;
    cout << "[Usage] wrap input_image_name [output_image_name] [wrap_mode] [wrap_parameter]" << endl;
    cout << "[wrap_mode] 1 - stretch image, 2 - twirl image" << endl;
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
  // default mode: twirl mode
  int mode = 2;
  double parameter = 2;

  // command line parser
  getCmdOptions(argc, argv, inputImage, outputImage, mode, parameter);
  
  // read input image
  readimage(inputImage);
  // wrap the original image
  wrapimage(mode, parameter);
  // write out to an output image file
  if (outputImage != "") {writeimage(outputImage);}
  
  // display input image and output image in seperated windows
  // start up the glut utilities
  glutInit(&argc, argv);
  
  // create the graphics window, giving width, height, and title text
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  int width = (xres > xres_out) ? xres : xres_out;
  int height = (yres > yres_out) ? yres : yres_out;
  glutInitWindowSize(width, height);

  // first window: input image
  glutCreateWindow("Input Image");  
  // set up the callback routines to be called when glutMainLoop() detects an event
  glutDisplayFunc(displayInput);	  // display callback
  glutMouseFunc(mouseClick);  // mouse callback
  glutReshapeFunc(handleReshape_in); // window resize callback

  // second window: output image
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


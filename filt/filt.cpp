/*
OpenGL and GLUT program to filter an input image and optionally write out the image to an image file.
Filter can be read from a filter file or generated from Gabor filter parameters.

Usage: 
- Filter an image from filter file  
  filt <input_image_file> <filter_file> <output_image_file>(optional)
- Filter an image by Gabor filter generated from parameters
  filt <input_image_file> <output_image_file>(optional) -g <theta> <sigma> <period>

Mouse Response:
  Left click any of the displayed windows to quit the program.

Jingcong Zhang
jingcoz@g.clemson.edu
2016-10-02
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


static unsigned char *inputpixmap;   // input image pixel map
static unsigned char *outputpixmap;   // output image pixel map
static double **kernel;   // 2D-array to store convolutional kernel
static int inputChannels;   // color channel number of input image
static int xres, yres;   // window size: image width, image height
static int kernel_size;   // kernel size


/*
reflect image at borders
*/
int reflectBorder(int index, int total)
{
  int index_new;
  index_new = index;  
  if (index < 0)  {index_new = -index;}
  if (index >= total) {index_new = 2 * (total - 1) - index;}

  return index_new;
}


/*
convolutional operation
*/
void conv(double **in, double **out)
{
  int n = (kernel_size - 1) / 2;
  for (int row = -n; row < yres - n; row++)
  {
    for (int col = -n; col < xres - n; col++)
    {
      double sum = 0;
      // inside boundary
      if (row <= (yres - kernel_size) and row >= 0 and col <= (xres - kernel_size) and col >= 0)
      {
        for (int i = 0; i < kernel_size; i++)
        {
          for (int j = 0; j < kernel_size; j++)
          {
            sum += kernel[kernel_size - 1 - i][kernel_size - 1 - j] * in[row + i][col + j];
          }
        }
      }
      // boundary conditions
      // reflect image at borders to add points for rows and cols outside the boundaries
      else
      {
        for (int i = 0; i < kernel_size; i++)
        {
          for (int j = 0; j < kernel_size; j++)
          {
            sum += kernel[kernel_size - 1 - i][kernel_size - 1 - j] * in[reflectBorder(row + i, yres)][reflectBorder(col + j, xres)];
          }
        }
      }
      out[row + n][col + n] = sum;
    }
  }
}


/*
filter image for each channels
*/
void filterImage()
{
  outputpixmap = new unsigned char [xres * yres * inputChannels];
  
  int m = (xres > yres) ? xres : yres;
  double **channel_value = new double *[m];
  double **out_value = new double *[m];
  for (int i = 0; i < m; i++)  
  {
    channel_value[i] = new double [m];
    out_value[i] = new double [m];
  }

  // seperate channel values
  for (int channel = 0; channel < inputChannels; channel++)
  {
    for (int row = 0; row < yres; row++)
    {
      for (int col = 0; col < xres; col++)
      {
        // store the channel value on scale 0-1
        channel_value[row][col] = float(inputpixmap[(row * xres + col) * inputChannels + channel]) / 255;
      }
    }
    conv(channel_value, out_value);
    for (int row = 0; row < yres; row++)
    {
      for (int col = 0; col < xres; col++)
      {
        // scale the output value to 0-255: 255 times the absolute value
        outputpixmap[(row * xres + col) * inputChannels + channel] = 255 * abs(out_value[row][col]);
      }
    }
  }
  
  // release memory
  for (int i = 0; i < m; i++)  {delete [] channel_value[i];  delete [] out_value[i];}
  delete [] channel_value;
  delete [] out_value;
}


/*
get filter kernel from filter file
*/
void readfilter(string filterfile)
{
  fstream filterFile(filterfile.c_str());
  // get kernel size
  double scale_factor;  // debug: scale_factor can't be int
  filterFile >> kernel_size >> scale_factor;
  // allocate memory for kernel 2D-array
  kernel = new double *[kernel_size];
  for (int i = 0; i < kernel_size; i++)
  {
    kernel[i] = new double [kernel_size];
  }
  // get kernel values
  double positive_sum = 0;
  double negative_sum = 0;
  cout << "Kernel Size: " << kernel_size << endl;
  cout << "Kernel " << filterfile << ": " << endl;
  for (int row = 0; row < kernel_size; row++)
  {
    for (int col = 0; col < kernel_size; col++)
    {
      filterFile >> kernel[row][col];
      cout << kernel[row][col] << " ";
      if (kernel[row][col] > 0) {positive_sum += kernel[row][col];}
      else  {negative_sum += (-kernel[row][col]);}
    }
    cout << endl;
  }
  // kernel normalization
  double scale = (positive_sum > negative_sum) ? positive_sum : negative_sum;
  cout << "Scale Factor: " << scale << endl;
  for (int row = 0; row < kernel_size; row++)
  {
    for (int col = 0; col < kernel_size; col++)
    {
      kernel[row][col] = kernel[row][col] / scale;
    }
  }
}


/*
calculate gabor filter kernel with a kernel size n/m = 2 * sigma 
*/
void getGaborFilter(double theta, double sigma, double T)
{
  int kernel_center;
  kernel_size = 4 * sigma + 1;
  kernel_center = 2 * sigma;
  kernel = new double *[kernel_size];
  for (int i = 0; i < kernel_size; i++)  {kernel[i] = new double [kernel_size];}
  
  cout << "Kernel Size: " << kernel_size << endl;
  cout << "Gabor Kernel: " << endl;
  double positive_sum = 0;
  double negative_sum = 0;
  for (int row = 0; row < kernel_size; row++)
  {
    for (int col = 0; col < kernel_size; col++)
    {
      double x, y, xx, yy;
      // calculate the distance to kernel center
      x = (col > 0) ? ((col + 0.5) - kernel_center) : ((col - 0.5) - kernel_center);
      y = (row > 0) ? ((row + 0.5) - kernel_center) : ((row - 0.5) - kernel_center);
   
      xx = x * cos(theta * M_PI / 180) + y * sin(theta * M_PI / 180);
      yy = -x * sin(theta * M_PI / 180) + y * cos(theta * M_PI / 180);
      kernel[row][col] = exp(-(pow(xx, 2.0) + pow(yy, 2.0)) / (2 * pow(sigma, 2.0))) * cos(2 * M_PI * xx / T);

      if (kernel[row][col] > 0) {positive_sum += kernel[row][col];}
      else  {negative_sum += (-kernel[row][col]);}

      cout << setprecision(2) << kernel[row][col] << " ";
    }
    cout << endl;
  }

  double scale = (positive_sum > negative_sum) ? positive_sum : negative_sum;
  cout << "Scale Factor: " << scale << endl;
  for (int row = 0; row < kernel_size; row++)
  {
    for (int col = 0; col < kernel_size; col++)
    {
      kernel[row][col] = kernel[row][col] / scale;
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
    inputChannels = spec.nchannels;
    cout << "Image Size: " << xres << "X" << yres << endl;

    cout << "channels: " << inputChannels << endl;

    inputpixmap = new unsigned char [xres * yres * inputChannels];
    in -> read_image(TypeDesc::UINT8, inputpixmap);

    in -> close();  // close the file
    delete in;    // free ImageInput
  }
}


/*
write out the associated color image from image pixel map
*/
void writeimage(string outfilename, int channels)
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
    cout << "Write the image pixmap to image file " << outfilename << endl;
    // close the file and free the ImageOutput I created
    out -> close();

    delete out;
  }
}


/*
display composed associated color image
*/
void display(const unsigned char *pixmap, int channels, int w, int h)
{
  // modify the pixmap: upside down the image
  unsigned char displaypixmap[w * h * channels];
  for (int i = 0; i < w; i++) // col
  {
    for (int j = 0; j < h; j++) // row
    {   
      for (int k = 0; k < channels; k++)
      {
        displaypixmap[(j * w + i) * channels + k] = pixmap[((h - 1 - j) * w + i) * channels + k];
      }
    }
  }

  // display the pixmap
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glRasterPos2i(0, 0);
  // glDrawPixels writes a block of pixels to the framebuffer.
  switch (channels)
  {
    case 1:
      glDrawPixels(w, h, GL_LUMINANCE, GL_UNSIGNED_BYTE, displaypixmap);
      break;
    case 3:
      glDrawPixels(w, h, GL_RGB, GL_UNSIGNED_BYTE, displaypixmap);
      break;
    case 4:
      glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, displaypixmap);
    default:
      return;  
  }

  glFlush();
}
void displayInput() {display(inputpixmap, inputChannels, xres, yres);}
void displayOutput()  {display(outputpixmap, inputChannels, xres, yres);}


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
command line option parser
  MODE1 - filter the image via Gabor filter: filt <input_image_name> <output_image_name>(optional) -g theta sigma period
  MODE2 - filter the image via filter from file: filt <input_image_name> <filter_name> <output_image_name>(optional)
*/
char **getIter(char** begin, char** end, const std::string& option) {return find(begin, end, option);}
void getCmdOption(int argc, char **argv, string &inputImage, string &filter, string &outImage, double &theta, double &sigma, double &T, int &mode)
{
  if (argc < 3)
  {
    cout << "Help: " << endl;
    cout << "Gabor Filter: " << endl;
    cout << "[Usage] filt <input_image_name> <output_image_name>(optional) -g theta sigma period" << endl;
    cout << "Filter File: " << endl;
    cout << "[Usage] filt <input_image_name> <filter_name> <output_image_name>(optional)" << endl;
    exit(0);
  }
  char **iter = getIter(argv, argv + argc, "-g");
  // gabor mode
  if (iter != argv + argc)
  {
    mode = 1; // mode 1: gabor filter mode
    cout << "Gabor Filter" << endl;
    if (argc >= 6)
    {
      if (++iter != argv + argc)
      {
        theta = atof(iter[0]);
        sigma = atof(iter[1]);
        T = atof(iter[2]);
        inputImage = argv[1];
        cout << "theta = " << theta << " sigma = " << sigma << " period = " << T << endl;
        if (argc == 7)  {outImage = argv[2];}
      }
      else  {cout << "Gabor Filter: " << endl << "[Usage] filt <input_image_name> -g theta sigma period" << endl; exit(0);}
    }
    else  {cout << "Gabor Filter: " << endl << "[Usage] filt <input_image_name> -g theta sigma period" << endl; exit(0);}
  }
  // normal mode
  else
  {
    mode = 2;
    cout << "Filter From File" << endl;
    if (argc >= 3)
    {
      inputImage = argv[1];
      filter = argv[2];
      cout << "Input Image: " << inputImage << endl;
      cout << "Filter File: " << filter << endl;
      if (argc == 4)  {outImage = argv[3];  cout << "Output Image: " << outImage << endl;}
    }
    else
    {
      cout << "Filter File: " << endl << "[Usage] filt <input_image_name> <filter_name> <output_image_name>(optional)" << endl;
      exit(0);
    }
  }
}


/*
Main program
*/
int main(int argc, char* argv[])
{
  string inputImage;    // input image file name
  string filter;    // filter file name
  string outImage;  // output image file name
  double theta;   // gabor filter parameter: theta
  double sigma;   // gabor filter parameter: sigma
  double T;       // gabor filter parameter: period
  int mode = 0; // mode = 1: gabor filter, mode = 2: filter from file

  // command line parser
  getCmdOption(argc, argv, inputImage, filter, outImage, theta, sigma, T, mode);
  
  // read input image
  readimage(inputImage);
  // filter image
  if (mode == 1)
  {
    getGaborFilter(theta, sigma, T);    
    filterImage();
  }
  if (mode == 2)
  {
    readfilter(filter);
    filterImage();
  }
  // write out to an output image file
  if (outImage != "") {writeimage(outImage, inputChannels);}
  
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

  // release memory
  delete [] inputpixmap;
  delete [] outputpixmap;
  for (int i = 0; i < kernel_size; i++)  {delete [] kernel[i];}
  delete [] kernel;

  return 0;
}


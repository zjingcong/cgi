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


static unsigned char *inputpixmap;   // input image pixel map
static unsigned char *outputpixmap;   // output image pixel map
static int inputChannels;   // color channel number of input image
static int xres, yres;   // window size: image width, image height
static double **kernel;   // 2D-array to store convolutional kernel
static int kernel_size;   // kernel size


/*
get filter kernel from filter file
*/
void readfilter(string filterfile)
{
  fstream filterFile(filterfile.c_str());
  // get kernel size
  int scale_factor;
  filterFile >> kernel_size >> scale_factor;
  // allocate memory for kernel 2D-array
  kernel = new double *[kernel_size];
  for (int i = 0; i < kernel_size; i++)
  {
    kernel[i] = new double [kernel_size];
  }
  // get kernel values
  double sum = 0;
  for (int row = 0; row < kernel_size; row++)
  {
    for (int col = 0; col < kernel_size; col++)
    {
      filterFile >> kernel[row][col];
      sum += kernel[row][col];
    }
  }
  // kernel normalization
  for (int row = 0; row < kernel_size; row++)
  {
    for (int col = 0; col < kernel_size; col++)
    {
      kernel[row][col] = kernel[row][col] / sum;
    }
  }
}


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
void conv(unsigned char **in, unsigned char **out)
{
  int n = (kernel_size - 1) / 2;
  for (int row = -n; row < xres - n; row++)
  {
    for (int col = -n; col < yres - n; col++)
    {
      double sum = 0;
      // inside boundary
      if (row <= (xres - kernel_size) and row >= 0 and col <= (yres - kernel_size) and col >= 0)
      {
        for (int i = 0; i < kernel_size; i++)
        {
          for (int j = 0; j < kernel_size; j++)
          {
            sum += kernel[i][j] * in[row + i][col + j];
          }
        }
      }
      // boundary conditions: reflect image at borders
      else
      {
        for (int i = 0; i < kernel_size; i++)
        {
          for (int j = 0; j < kernel_size; j++)
          {
            sum += kernel[i][j] * in[reflectBorder(row + i, xres)][reflectBorder(col + j, yres)];
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

  unsigned char **channel_value = new unsigned char *[xres];
  unsigned char **out_value = new unsigned char *[xres];
  for (int i = 0; i < yres; i++)
  {
    channel_value[i] = new unsigned char [yres];
    out_value[i] = new unsigned char [yres];
  }
  // seperate channel values
  for (int channel = 0; channel < inputChannels; channel++)
  {
    for (int row = 0; row < xres; row++)
    {
      for (int col = 0; col < yres; col++)
      {
        channel_value[row][col] = inputpixmap[(col * xres + row) * inputChannels + channel];
      }
    }
    conv(channel_value, out_value);
    for (int row = 0; row < xres; row++)
    {
      for (int col = 0; col < yres; col++)
      {
        outputpixmap[(col * xres + row) * inputChannels + channel] = out_value[row][col];
      }
    }
  }
  
  // release memory
  for (int i = 0; i < yres; i++)
  {
    delete [] channel_value[i];
    delete [] out_value[i];
  }
  delete [] channel_value;
  delete [] out_value;
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

    cout << "channels: " << inputChannels << endl;

    inputpixmap = new unsigned char [xres * yres * inputChannels];
    in -> read_image(TypeDesc::UINT8, inputpixmap);

    in -> close();  // close the file
    delete in;    // free ImageInput
  }
}


/*
display composed associated color image
*/
void display(const unsigned char *pixmap, int channels)
{
  // modify the pixmap: upside down the image
  // int n;
  // n = (channels > 3) ? 3 : channels;
  unsigned char displaypixmap[xres * yres * channels];
  for (int i = 0; i < xres; i++)
  {
    for (int j = 0; j < yres; j++)
    {   
      for (int k = 0; k < channels; k++)
      {
        displaypixmap[(j * xres + i) * channels + k] = pixmap[((yres - 1 - j) * xres + i) * channels + k];
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
      glDrawPixels(xres, yres, GL_LUMINANCE, GL_UNSIGNED_BYTE, displaypixmap);
      break;
    case 3:
      glDrawPixels(xres, yres, GL_RGB, GL_UNSIGNED_BYTE, displaypixmap);
      break;
    case 4:
      glDrawPixels(xres, yres, GL_RGBA, GL_UNSIGNED_BYTE, displaypixmap);
    default:
      return;  
  }

  glFlush();
}


void displayInput()
{
  display(inputpixmap, inputChannels);
}


void displayOutput()
{
  display(outputpixmap, inputChannels);
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
  string inputImage;    // input image file name
  string filter;    // filter file name
  string outImage;  // output image file name

  // command line: get the filter, input image and output image optionally
  // usage: filt <input_image_name> <filter_name> <output_image_name>(optional)
  if (argc >= 3)
  {
    cout << "Input image file name: " << argv[1] << endl;
    cout << "Filter file name: " << argv[2] << endl;
    inputImage = argv[1];
    filter = argv[2];
    if (argc == 4)
      {
        cout << "Output image file name: " << argv[3] << endl;
        outImage = argv[3];
      }
  }
  else
  {
    cout << "[Usage] filt <input_image_name> <filter_name> <output_image_name>(optional)" << endl;
    return 0;
  }

  readimage(inputImage);
  readfilter(filter);
  filterImage();
  
  // start up the glut utilities
  glutInit(&argc, argv);
  
  // create the graphics window, giving width, height, and title text
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  glutInitWindowSize(xres, yres);

  // first window: input image
  glutCreateWindow("Input Image");  
  // set up the callback routines to be called when glutMainLoop() detects an event
  glutDisplayFunc(displayInput);	  // display callback
  // glutKeyboardFunc(handleKey);	  // keyboard callback
  glutReshapeFunc(handleReshape); // window resize callback

  // second window: output image
  glutCreateWindow("Output Image");  
  // set up the callback routines to be called when glutMainLoop() detects an event
  glutDisplayFunc(displayOutput);	  // display callback
  // glutKeyboardFunc(handleKey);	  // keyboard callback
  glutReshapeFunc(handleReshape); // window resize callback
  
  // Routine that loops forever looking for events. It calls the registered
  // callback routine to handle each event that is detected
  glutMainLoop();

  delete [] inputpixmap;
  delete [] outputpixmap;

  return 0;
}


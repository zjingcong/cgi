# include <OpenImageIO/imageio.h>
# include <stdlib.h>
# include <cstdlib>
# include <iostream>
# include <fstream>
# include <string>
# include <algorithm>

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


void displayInput() {display(inputpixmap, inputChannels);}
void displayOutput()  {display(outputpixmap, inputChannels);}


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
    mode = 1;
    cout << "Gabor Filter" << endl;
    if (argc >= 6)
    {
      if (++iter != argv + argc)
      {
        theta = atof(iter[0]);
        sigma = atof(iter[1]);
        T = atof(iter[2]);
        inputImage = argv[1];
        cout << theta << " " << sigma << " " << T << endl;
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
      outImage = argv[3];
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
  double theta;
  double sigma;
  double T;
  int mode = 0; // mode = 1: gabor filter, mode = 2: filter from file

  // command line parser
  getCmdOption(argc, argv, inputImage, filter, outImage, theta, sigma, T, mode);

  readimage(inputImage);
  if (mode == 2)
  {
    readfilter(filter);
    filterImage();
  }  
  if (outImage != "") {writeimage(outImage, inputChannels);}
  
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
  glutMouseFunc(mouseClick);  // mouse callback
  glutReshapeFunc(handleReshape); // window resize callback

  // second window: output image
  glutCreateWindow("Output Image");  
  // set up the callback routines to be called when glutMainLoop() detects an event
  glutDisplayFunc(displayOutput);	  // display callback
  // glutKeyboardFunc(handleKey);	  // keyboard callback
  glutMouseFunc(mouseClick);  // mouse callback
  glutReshapeFunc(handleReshape); // window resize callback
  
  // Routine that loops forever looking for events. It calls the registered
  // callback routine to handle each event that is detected
  glutMainLoop();

  delete [] inputpixmap;
  delete [] outputpixmap;

  return 0;
}


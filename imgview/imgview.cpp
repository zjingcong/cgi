# include <OpenImageIO/imageio.h>

# include <cstdlib>
# include <iostream>
# include <string>

# ifdef __APPLE__
#   pragma clang diagnostic ignored "-Wdeprecated-declarations"
#   include <GLUT/glut.h>
# else
#   include <GL/glut.h>
# endif

using namespace std;
OIIO_NAMESPACE_USING

# define WIDTH	    600	// window dimensions
# define HEIGHT		600

static unsigned char pixmap[600 * 600 * 4] = {0};   // pixmap read from the input image
static unsigned char new_pixmap[600 * 600 * 4] = {0};   // modified pixmap
static int xres;    // width
static int yres;    // height
static int channels;    // channel number
static string infilename;   // input file name

/*
get the image pixmap
*/
void inputimage()
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
        xres = spec.width;
        yres = spec.height;
        channels = spec.nchannels;
        
        // convert the RGB image to RGBA image
        if (channels == 3)
        {
            unsigned char tmp[xres * yres * channels];
            in -> read_image(TypeDesc::UINT8, tmp);  // store the pixmap
            int i, j, k; 
            for (i = 0; i < xres; i++)
            {
                for (j = 0; j < yres; j++)
                {   
                    pixmap[(j * xres + i) * 4 + 3] = 255;   // set the A channel value to 255 for each pixel
                    for (k = 0; k < 3; k++)
                    {
                        pixmap[(j * xres + i) * 4 + k] = tmp[(j * xres + i) * 3 + k];
                    }
                }
            }
        }
        else    {in -> read_image(TypeDesc::UINT8, pixmap);}  // store the pixmap
            
        in -> close();  // close the file
        delete in;    // free ImageInput
    }
}

/*
Routine to read an image file
*/
void readimage()
{
    // get the file name of the input image file
    string filename;
    cout << "enter input image filename: ";
    cin >> filename;
    infilename = filename;
    
    inputimage();
}

/*
Routine to get image from OpenGL framebuffer and then write to an image file
*/
void writeimage()
{
    string outfilename;
    cout << "enter output image filename: ";
    cin >> outfilename;

    // get the current image from the OpenGL framebuffer and store in pixmap
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, glpixmap);
    
    // if is ppm file
    bool isppm = 0;
    unsigned char ppmpixmap[xres * yres * 4];
    if (outfilename.substr(outfilename.find_last_of(".") + 1, outfilename.length() - 1) == "ppm")   
    {
        isppm = 1;
        int i, j, k; 
        for (i = 0; i < xres; i++)
        {
            for (j = 0; j < yres; j++)
            {   
                for (k = 0; k < 3; k++)
                {
                    ppmpixmap[(j * xres + i) * 3 + k] = glpixmap[(j * xres + i) * 4 + k];
                }
            }
        }
    } 

    
    ImageOutput *out = ImageOutput::create(outfilename);
    if (!out)
    {
        cerr << "Could not create output image for " << outfilename << ", error = " << geterror() << endl;
    }
    else
    {   
        ImageSpec spec (xres, yres, 4, TypeDesc::UINT8);
        out -> open(outfilename, spec);
        if (isppm)  {out -> write_image(TypeDesc::UINT8, ppmpixmap);}
        else {out -> write_image(TypeDesc::UINT8, glpixmap);}
        cout << "Write the image pixmap to image file " << outfilename << endl;
        out -> close();
        delete out;
    }
}

void display()
{   
    // modify the pixmap: upside down the image
    int i, j, k; 
    for (i = 0; i < xres; i++)
    {
        for (j = 0; j < yres; j++)
        {   
            for (k = 0; k < 4; k++)
            {
                new_pixmap[(j * xres + i) * 4 + k] = pixmap[((yres - 1 - j) * xres + i) * 4 + k];
            }
        }
    }

    
    // display the pixmap
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glRasterPos2i(0, 0);
    // glDrawPixels writes a block of pixels to the framebuffer.
    glDrawPixels(xres, yres, GL_RGBA, GL_UNSIGNED_BYTE, new_pixmap);
    glFlush();
}

/*
Keyboard Callback Routine: 'r' or 'R' read an image file, 'w' or 'W' write the pixmap to an image file, 'q', 'Q' or ESC quit
This routine is called every time a key is pressed on the keyboard
*/
void handleKey(unsigned char key, int x, int y)
{
    switch(key){
        case 'r':
        case 'R':
            readimage();
            glutPostRedisplay();
            break;

        case 'w':
        case 'W':
            writeimage();
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
This routine is called when the window is created and every time the window
is resized, by the program or by the user
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
        if (xfactor > yfactor)  {factor = yfactor;}
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
    int w = WIDTH;
    int h = HEIGHT;
    // optional command line
    if (argc == 2)  // one argument of image file name
    {
        cout << "Image file name: " << argv[1] << endl;
        infilename = argv[1];
        inputimage();
        w = xres;
        h = yres;
    }
  
    // start up the glut utilities
    glutInit(&argc, argv);
  
    // create the graphics window, giving width, height, and title text
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
    glutInitWindowSize(w, h);
    glutCreateWindow("My Image View");
  
    // set up the callback routines to be called when glutMainLoop() detects
    // an event
    glutDisplayFunc(display);	  // display callback
    glutKeyboardFunc(handleKey);	  // keyboard callback
    glutReshapeFunc(handleReshape); // window resize callback
  
    // Routine that loops forever looking for events. It calls the registered
    // callback routine to handle each event that is detected
    glutMainLoop();
    return 0;
}


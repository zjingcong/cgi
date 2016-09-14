// VIZA 654 HW2
// Albers Color Studies
// coded by Cem Yuksel
//
// Tuned by Donald House
// for ART 803, Jan. 7, 2009
// for OpenImageIO, June 28, 2016
//
// MODES:
//   Color Edit Mode (enter via F1)
//   View Mode (enter via F2)
//   Shape Mode (enter via F3)
//
// COLOR EDITING
//   Left Mouse Drag
//     hue change - left/right or down/up
//   Shift Left Mouse Drag
//     value change - down/up
//     saturation change - left/right
//   1 Key - Edit Left Field
//   2 Key - Edit Right Field
//   3 Key - Edit Squares
//
// ACTIVATE MENUS
//   Right Mouse
//

#ifdef __APPLE__
#  pragma clang diagnostic ignored "-Wdeprecated-declarations"
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

#include <cstdio>
#include <cstring>
#include <sys/time.h>
#include <ctime>

#include <OpenImageIO/imageio.h>
#include "project.h"
#include "colorwindow.h"

#define WIDTH 1024
#define HEIGHT	512

int windowWidth = WIDTH;
int windowHeight = HEIGHT;
float windowAspect = (float)windowHeight / (float)windowWidth;
int mainWindow, subWindow;

int userInputMode = 0;
#define MAX_USER_TEXT 512
char userInputTitle[ MAX_USER_TEXT ] = "Enter file name to open";
char userInputText[ MAX_USER_TEXT ] = "";
int userFeedbackMode = 0;
#define USER_FEEDBACK_TIME 2000
char userFeedbackText[ MAX_USER_TEXT ] = "File Saved.";
char helpText[5][MAX_USER_TEXT];

#define MODE_VIEW 		101
#define MODE_COLOR		102
#define MODE_SHAPE		103
int mode = MODE_COLOR;

#define IMAGE_FORMAT_JPG 10
#define IMAGE_FORMAT_PPM 20

enum Commands {
  CMD_EXIT=0,
  CMD_LOAD_PROJECT,
  CMD_SAVE_PROJECT,
  CMD_SAVE_IMAGE,
  CMD_HELP,
  CMD_COLORS,
  CMD_MIRROR,
  CMD_4COLORS,
  CMD_PLAY_ANIMATION,
  CMD_EDIT_LEFT_COLOR,
  CMD_EDIT_RIGHT_COLOR,
  CMD_EDIT_SHAPE_COLOR,
  CMD_EDIT_LEFT_SHAPE_COLOR,
  CMD_SHAPE_RECTANGLE,
  CMD_SHAPE_CIRCLE,
  CMD_SHAPE_POLYGON,
  CMD_MOVEMENT_CLEAR
};
//#define CMD_EXPORT_IMAGE 10000

enum ImageFormats {
  UNKNOWN = 0,
  JPG,
  PPM
};

bool shiftOn = false;
bool ctrlOn = false;
bool altOn = false;

// MENU
int menu, viewmodemenu, colormodemenu, shapemodemenu;

bool fullScreen = false;
int winOwidth = WIDTH, winOheight = HEIGHT;
int mouseX, mouseY;

#define TOSCENEX(x) ( float ( 2 * x ) / float ( windowWidth ) - 1.0f )
#define TOSCENEY(y) ( float ( 2 * y - windowHeight + windowWidth ) / float ( windowWidth ) - 1.0f )
#define TIMEDIF( t2, t1 ) ( ( t2.tv_sec - t1.tv_sec )*1000 + ( t2.tv_usec - t1.tv_usec ) / 1000 )

#define MIN_RECORD_TIME_DIF 30	// in milliseconds

bool showHelp = false;
bool showColors = false;
timeval lastDisplayTime;
RecordKey *currentRecordKey = NULL;
bool followMouse = false;
bool recording = false;
timeval lastRecordTime;

Project prj;
//Color3 *selectedColor = &prj.leftColor;
HSV_Color *selectedColor = &prj.leftColor;

using namespace std;
OIIO_NAMESPACE_USING

//CVImage image;

void DisplayNoHelp();

//
// Routine to write the current framebuffer to an image file
//
int writeimage(string outfilename){
  int pixformat;
  int retval = 1;
  
  // make a pixmap that is the size of the window and grab framebuffer into it
  int WinWidth = glutGet(GLUT_WINDOW_WIDTH);
  int WinHeight = glutGet(GLUT_WINDOW_HEIGHT);
  
  // make a pixmap that is the size of the window and grab OpenGL framebuffer into it
  // this assumes that OpenGL is using RGBA display format
  unsigned char local_pixmap[WinWidth * WinHeight * 4];
  glReadPixels(0, 0, WinWidth, WinHeight, GL_RGBA, GL_UNSIGNED_BYTE, local_pixmap);
  
  // create the oiio file handler for the image
  ImageOutput *outfile = ImageOutput::create(outfilename);
  if(!outfile){
    cerr << "Could not create output image for " << outfilename << ", error = " << geterror() << endl;
    return 0;
  }
  
  // Open a file for writing the image. The file header will indicate an image of
  // width WinWidth, height WinHeight, and ImChannels channels per pixel.
  // All channels will be of type unsigned char
  ImageSpec spec(WinWidth, WinHeight, 4, TypeDesc::UINT8);
  if(!outfile->open(outfilename, spec)){
    cerr << "Could not open " << outfilename << ", error = " << geterror() << endl;
    delete outfile;
    return 0;
  }
  
  // Write the image to the file. All channel values in the pixmap are taken to be
  // unsigned chars. While writing, flip the image upside down, since OpenGL
  // pixmaps have the bottom scanline first in the array, and oiio writes the top
  // scanline first in the image file.
  int scanlinesize = WinWidth * 4 * sizeof(unsigned char);
  if(!outfile->write_image(TypeDesc::UINT8, local_pixmap + (WinHeight - 1) * scanlinesize, AutoStride, -scanlinesize)){
    cerr << "Could not write image to " << outfilename << ", error = " << geterror() << endl;
    retval = 0;
  }
  
  // close the image file after the image is written and free up space for the
  // ooio file handler
  outfile->close();
  delete outfile;
  return retval;
}

void RenderText ( char *text )
{
  for ( int i=0; i<MAX_USER_TEXT; i++ ) {
    if ( text[i] == '\0' ) break;
    glutBitmapCharacter( GLUT_BITMAP_HELVETICA_18, text[i] );
  }
}

void RenderText2 ( char *text )
{
  for ( int i=0; i<MAX_USER_TEXT; i++ ) {
    if ( text[i] == '\0' ) break;
    glutBitmapCharacter( GLUT_BITMAP_8_BY_13, text[i] );
  }
}

void RenderText3 ( char *text )
{
  for ( int i=0; i<MAX_USER_TEXT; i++ ) {
    if ( text[i] == '\0' ) break;
    glutBitmapCharacter( GLUT_BITMAP_HELVETICA_10, text[i] );
  }
}

void DisplayNoHelp() {
  unsigned char leftColor[3], rightColor[3], shapeColor[3];
  
  HSVtoRGB (prj.leftColor.h, prj.leftColor.s, prj.leftColor.v, leftColor[0], leftColor[1], leftColor[2]);
  HSVtoRGB (prj.rightColor.h, prj.rightColor.s, prj.rightColor.v, rightColor[0], rightColor[1], rightColor[2]);
  HSVtoRGB (prj.shapeColor.h, prj.shapeColor.s, prj.shapeColor.v, shapeColor[0], shapeColor[1], shapeColor[2]);
  
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  
  int windowMid = windowWidth / 2;
  
  glPushMatrix();
  
  glLoadIdentity();
  
  // Display BG Colors
  glBegin ( GL_QUADS );
  glColor3ubv ( leftColor );
  glVertex2f ( -1, -windowAspect );
  glVertex2f ( 0, -windowAspect );
  glVertex2f ( 0,  windowAspect );
  glVertex2f ( -1, windowAspect );
  glColor3ubv ( rightColor );
  glVertex2f ( 0, -windowAspect );
  glVertex2f ( 1, -windowAspect );
  glVertex2f ( 1,  windowAspect );
  glVertex2f ( 0, windowAspect );
  glEnd();
  
  // Display Shape
  glPushMatrix();
  Vertex *vert = prj.shape.GetVerts();
  glColor3ubv ( shapeColor );
  glTranslatef ( prj.shape.posX, prj.shape.posY, 0 );
  glScalef ( prj.shape.scaleX, prj.shape.scaleY, 1 );
  glBegin ( GL_POLYGON );
  while ( vert != NULL ) {
    glVertex2fv ( vert->v );
    vert = vert->Next;
  }
  glEnd();
  glPopMatrix();
  
  // Display Mirror Shape
  if ( prj.displayMirror ) {
    glPushMatrix();
    Vertex *vert = prj.shape.GetVerts();
    glColor3ubv ( shapeColor );
    glTranslatef ( -prj.shape.posX, prj.shape.posY, 0 );
    glScalef ( -prj.shape.scaleX, prj.shape.scaleY, 1 );
    glBegin ( GL_POLYGON );
    while ( vert != NULL ) {
      glVertex2fv ( vert->v );
      vert = vert->Next;
    }
    glEnd();
    glPopMatrix();
  }
  
  glPopMatrix();
}

void Display()
{
  unsigned char leftColor[3], rightColor[3], shapeColor[3], leftShapeColor[3];
  
  HSVtoRGB (prj.leftColor.h, prj.leftColor.s, prj.leftColor.v, leftColor[0], leftColor[1], leftColor[2]);
  HSVtoRGB (prj.rightColor.h, prj.rightColor.s, prj.rightColor.v, rightColor[0], rightColor[1], rightColor[2]);
  HSVtoRGB (prj.shapeColor.h, prj.shapeColor.s, prj.shapeColor.v, shapeColor[0], shapeColor[1], shapeColor[2]);
  HSVtoRGB (prj.leftShapeColor.h, prj.leftShapeColor.s, prj.leftShapeColor.v, leftShapeColor[0], leftShapeColor[1], leftShapeColor[2]);
  
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  
  int windowMid = windowWidth / 2;
  
  glPushMatrix();
  
  glLoadIdentity();
  
  // Display BG Colors
  glBegin ( GL_QUADS );
  glColor3ubv ( leftColor );
  glVertex2f ( -1, -windowAspect );
  glVertex2f ( 0, -windowAspect );
  glVertex2f ( 0,  windowAspect );
  glVertex2f ( -1, windowAspect );
  glColor3ubv ( rightColor );
  glVertex2f ( 0, -windowAspect );
  glVertex2f ( 1, -windowAspect );
  glVertex2f ( 1,  windowAspect );
  glVertex2f ( 0, windowAspect );
  glEnd();
  
  // Display Shape
  glPushMatrix();
  Vertex *vert = prj.shape.GetVerts();
  glColor3ubv ( shapeColor );
  glTranslatef ( prj.shape.posX, prj.shape.posY, 0 );
  glScalef ( prj.shape.scaleX, prj.shape.scaleY, 1 );
  glBegin ( GL_POLYGON );
  while ( vert != NULL ) {
    glVertex2fv ( vert->v );
    vert = vert->Next;
  }
  glEnd();
  glPopMatrix();

  // Display Left Shape
  if ( prj.is4colorproblem) {
    glPushMatrix();
    Vertex *leftVert = prj.leftShape.GetVerts();
    glColor3ubv ( leftShapeColor );
    glTranslatef ( prj.leftShape.posX, prj.leftShape.posY, 0 );
    glScalef ( prj.leftShape.scaleX, prj.leftShape.scaleY, 1 );
    glBegin ( GL_POLYGON );
    while ( leftVert != NULL ) {
        glVertex2fv ( leftVert->v );
        leftVert = leftVert->Next;
    }
    glEnd();
    glPopMatrix();
  }
  
  // Display Mirror Shape
  if ( prj.displayMirror ) {
    glPushMatrix();
    Vertex *vert = prj.shape.GetVerts();
    glColor3ubv ( shapeColor );
    glTranslatef ( -prj.shape.posX, prj.shape.posY, 0 );
    glScalef ( -prj.shape.scaleX, prj.shape.scaleY, 1 );
    glBegin ( GL_POLYGON );
    while ( vert != NULL ) {
      glVertex2fv ( vert->v );
      vert = vert->Next;
    }
    glEnd();
    glPopMatrix();
  }
  
  glPopMatrix();
  
  if ( showColors ) {
    glMatrixMode ( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D ( 0, windowWidth, windowHeight, 0 );
    
    glColor3f ( 0, 0, 0 );
    char colortext[512];
    float h, s, v;
    
    sprintf ( colortext, "Left color:  R=%3d G=%3d B=%3d  H=%3.0f S=%4.2f V=%4.2f", leftColor[0], leftColor[1], leftColor[2], prj.leftColor.h, prj.leftColor.s, prj.leftColor.v );
    glRasterPos2i ( 10, 20 );
    RenderText2 ( colortext );
    
    sprintf ( colortext, "Right color: R=%3d G=%3d B=%3d  H=%3.0f S=%4.2f V=%4.2f", rightColor[0], rightColor[1], rightColor[2], prj.rightColor.h, prj.rightColor.s, prj.rightColor.v );
    glRasterPos2i ( 10, 36 );
    RenderText2 ( colortext );
    
    sprintf ( colortext, "Shape color: R=%3d G=%3d B=%3d  H=%3.0f S=%4.2f V=%4.2f", shapeColor[0], shapeColor[1], shapeColor[2], prj.shapeColor.h, prj.shapeColor.s, prj.shapeColor.v );
    glRasterPos2i ( 10, 52 );
    RenderText2 ( colortext );
    
    glPopMatrix();
    glMatrixMode ( GL_MODELVIEW );
  }
  
  if ( showHelp ) {
    glMatrixMode ( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D ( 0, windowWidth, windowHeight, 0 );
    
    glColor3f ( 0, 0, 0 );
    float h, s, v;
    
    glRasterPos2i ( 10, 80 );
    RenderText3 ( helpText[0] );
    glRasterPos2i ( 10, 96 );
    RenderText3 ( helpText[1] );
    glRasterPos2i ( 10, 112 );
    RenderText3 ( helpText[2] );
    glRasterPos2i ( 10, 128 );
    RenderText3 ( helpText[3] );
    glRasterPos2i ( 10, 144 );
    RenderText3 ( helpText[4] );
    
    glPopMatrix();
    glMatrixMode ( GL_MODELVIEW );
  }
  
  if ( userInputMode ) {
    glMatrixMode ( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D ( 0, windowWidth, windowHeight, 0 );
    
    glColor3f ( 1, 0, 0 );
    glBegin ( GL_POLYGON );
    glVertex2f ( 0, windowHeight - 20 );
    glVertex2f ( windowWidth, windowHeight - 20 );
    glVertex2f ( windowWidth, windowHeight - 40 );
    glVertex2f ( 0, windowHeight - 40 );
    glEnd();
    glColor3f ( 1, 1, 1 );
    glBegin ( GL_POLYGON );
    glVertex2f ( 0, windowHeight );
    glVertex2f ( windowWidth, windowHeight );
    glVertex2f ( windowWidth, windowHeight - 20 );
    glVertex2f ( 0, windowHeight - 20 );
    glEnd();
    glRasterPos2i ( 10, windowHeight - 23 );
    RenderText ( userInputTitle );
    glColor3f ( 0, 0, 0 );
    glRasterPos2i ( 10, windowHeight - 3 );
    RenderText ( userInputText );
    
    glPopMatrix();
    glMatrixMode ( GL_MODELVIEW );
  }
  
  if ( userFeedbackMode ) {
    glMatrixMode ( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D ( 0, windowWidth, windowHeight, 0 );
    
    glColor3f ( 1, 0, 0 );
    glBegin ( GL_POLYGON );
    glVertex2f ( 0, windowHeight );
    glVertex2f ( windowWidth, windowHeight );
    glVertex2f ( windowWidth, windowHeight - 20 );
    glVertex2f ( 0, windowHeight - 20 );
    glEnd();
    glColor3f ( 1, 1, 1 );
    glRasterPos2i ( 10, windowHeight - 3 );
    RenderText ( userFeedbackText );
    
    glPopMatrix();
    glMatrixMode ( GL_MODELVIEW );
    
    userFeedbackMode--;
  }
  
  glutSwapBuffers();
}

void Reshape( int w, int h )
{
  glViewport ( 0, 0, w, h );
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  windowAspect = (float) h / (float) w;
  gluOrtho2D ( -1, 1, windowAspect, -windowAspect );
  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();
  
  windowWidth = w;
  windowHeight = h;
  //glutPostRedisplay();
}

//void Idle ()
//{
  //glutPostRedisplay();
  //}

void UserInputDone (int mymode)
{
  switch ( mymode ) {
    case CMD_LOAD_PROJECT:
      if ( prj.Load ( userInputText ) ) {
        currentRecordKey = prj.record.GetFirstKey();
      } else {
        sprintf ( userFeedbackText, "%s cannot be loaded!", userInputText );
        userFeedbackMode = USER_FEEDBACK_TIME;
      }
      break;
    case CMD_SAVE_PROJECT:
      if ( ! prj.Save ( userInputText ) ) {
        sprintf ( userFeedbackText, "%s cannot be saved!", userInputText );
        userFeedbackMode = USER_FEEDBACK_TIME;
      }
      break;
    case CMD_SAVE_IMAGE:
      if ( ! writeimage ( userInputText ) ) {
        sprintf ( userFeedbackText, "%s cannot be saved!", userInputText );
        userFeedbackMode = USER_FEEDBACK_TIME;
      }
      break;
      
    default:
      break;
      /*
       if ( userInputMode >= CMD_EXPORT_IMAGE ) {
       if ( ! ExportImage ( userInputText, userInputMode - CMD_EXPORT_IMAGE ) ) {
       sprintf ( userFeedbackText, "%s cannot be saved!", userInputText );
       userFeedbackMode = USER_FEEDBACK_TIME;
       }
       }
       */
  }
}

void Menu( int item )
{
  switch ( item ) {
      
    case CMD_LOAD_PROJECT:
      sprintf ( userInputTitle, "Enter project file name to load." );
      sprintf ( userInputText, "" );
      userInputMode = CMD_LOAD_PROJECT;
      break;
    case CMD_SAVE_PROJECT:
      sprintf ( userInputTitle, "Enter project file name to save." );
      sprintf ( userInputText, "" );
      userInputMode = CMD_SAVE_PROJECT;
      break;
    case CMD_SAVE_IMAGE:
      sprintf ( userInputTitle, "Enter image file name to save." );
      sprintf ( userInputText, "" );
      userInputMode = CMD_SAVE_IMAGE;
      break;
    case CMD_HELP:
      showHelp = ! showHelp;
      break;
    case CMD_COLORS:
      showColors = ! showColors;
      break;
    case CMD_MIRROR:
      if ( !prj.is4colorproblem )
        { prj.displayMirror = !prj.displayMirror; }
      break;
    case CMD_4COLORS:
      prj.is4colorproblem = !prj.is4colorproblem;
      if (prj.is4colorproblem) {prj.displayMirror = false;}
      else {prj.displayMirror = true;}
      cout << prj.is4colorproblem << prj.displayMirror << endl;
      break;
      
    case MODE_VIEW:
      glutSetMenu ( menu );
      glutChangeToSubMenu ( 4, "View Mode Options", viewmodemenu );
      mode = MODE_VIEW;
      currentRecordKey = prj.record.GetFirstKey();
      sprintf ( helpText[0], "Current Mode is VIEW MODE." );
      sprintf ( helpText[1], "Left-click and drag to move the shape." );
      sprintf ( helpText[2], "Right-cick menu for View Mode Options." );
      sprintf ( helpText[3], "" );
      sprintf ( helpText[4], "" );
      break;
    case MODE_COLOR:
      glutSetMenu ( menu );
      glutChangeToSubMenu ( 4, "Edit Color Mode Options", colormodemenu );
      mode = MODE_COLOR;
      sprintf ( helpText[0], "Current Mode is COLOR MODE" );
      sprintf ( helpText[1], "Left-click and drag to change hue of the selected color." );
      sprintf ( helpText[2], "Left-click with SHIFT and drag to change value and saturation of the selected color." );
      sprintf ( helpText[3], "Press 1 to edit left, 2 for right, 3 for center in 3 color problems and 3 for center left, 4 for center right in 4 color problems." );
      sprintf ( helpText[4], "Right-cick menu for Color Mode Options." );
      break;
    case MODE_SHAPE:
      glutSetMenu ( menu );
      glutChangeToSubMenu ( 4, "Edit Shape Mode Options", shapemodemenu );
      mode = MODE_SHAPE;
      sprintf ( helpText[0], "Current Mode is SHAPE MODE." );
      sprintf ( helpText[1], "Left-click and drag to change shape size." );
      sprintf ( helpText[2], "Right-cick menu for Shape Mode Options." );
      sprintf ( helpText[3], "" );
      sprintf ( helpText[4], "" );
      break;
      
    case CMD_PLAY_ANIMATION:
      break;
      
    case CMD_EDIT_LEFT_COLOR:
      selectedColor = &prj.leftColor;
      break;
    case CMD_EDIT_RIGHT_COLOR:
      selectedColor = &prj.rightColor;
      break;
    case CMD_EDIT_SHAPE_COLOR:
      selectedColor = &prj.shapeColor;
      break;
    case CMD_EDIT_LEFT_SHAPE_COLOR:
      selectedColor = &prj.leftShapeColor;
      
    case CMD_SHAPE_RECTANGLE:
      prj.shape.SetRectangle();
      break;
    case CMD_SHAPE_CIRCLE:
      prj.shape.SetCircle();
      break;
    case CMD_SHAPE_POLYGON:
      break;
      
    case CMD_MOVEMENT_CLEAR:
      prj.record.Clear();
      currentRecordKey = NULL;
      break;
      
    case CMD_EXIT: // Exit
      prj.record.Clear();
      exit(0);
      break;
      
    default:
      /*
       if ( item >= CMD_EXPORT_IMAGE ) {
       sprintf ( userInputTitle, "Enter image file name to export." );
       sprintf ( userInputText, "" );
       userInputMode = item;
       }
       */
      break;
  }
  glutPostRedisplay();
}

void Keyboard ( unsigned char key, int x, int y )
{
  int mymode = userInputMode;
  
  if ( mymode ) {
    if ( key >= 32 && key <= 127 ) {
      int i;
      for ( i=0; i<MAX_USER_TEXT-1; i++ ) {
        if ( userInputText [ i ] == '\0' ) break;
      }
      userInputText [ i ] = key;
      userInputText [ i + 1 ] = '\0';
    } else {
      switch ( key ) {
        case 8:	//Backspace
        {
          int i;
          for ( i=0; i<MAX_USER_TEXT-1; i++ ) {
            if ( userInputText [ i ] == '\0' ) break;
          }
          if ( i > 0 ) userInputText[i-1] = '\0';
        }
          break;
        case 13:	// Enter
          userInputMode = 0;
          Display();
          UserInputDone(mymode);
          break;
        case 27:	// Escape
          userInputMode = 0;
          break;
      }
    }
  } else if ( userFeedbackMode ) {
    userFeedbackMode = 0;
  } else {
    switch ( key ) {
        
      case 'h': case 'H':
        Menu ( CMD_HELP );
        break;
      case 'c': case 'C':
        Menu ( CMD_COLORS );
        break;
      case 'm': case 'M':
        Menu ( CMD_MIRROR );
        break;
      case 'f': case 'F':
        Menu ( CMD_4COLORS );
        break;
        
      case '1':
        Menu ( CMD_EDIT_LEFT_COLOR );
        break;
      case '2':
        Menu ( CMD_EDIT_RIGHT_COLOR );
        break;
      case '3':
        Menu ( CMD_EDIT_SHAPE_COLOR );
        break;
      case '4':
        Menu ( CMD_EDIT_LEFT_SHAPE_COLOR );
        break;
        
      case 'q': case 'Q':
      case 27:	// Escape
        Menu(CMD_EXIT);
        break;
    }
  }
  glutPostRedisplay();
}

void Keyboard2 ( int key, int x, int y )
{
  switch ( key ) {
    case GLUT_KEY_F1:
      Menu ( MODE_COLOR );
      break;
    case GLUT_KEY_F2:
      Menu ( MODE_VIEW );
      break;
    case GLUT_KEY_F3:
      Menu ( MODE_SHAPE );
      break;
  }
}

void Mouse ( int button, int state, int x, int y )
{
  shiftOn = glutGetModifiers() & GLUT_ACTIVE_SHIFT;
  ctrlOn = glutGetModifiers() & GLUT_ACTIVE_CTRL;
  altOn = glutGetModifiers() & GLUT_ACTIVE_ALT;
  
  if ( button == GLUT_LEFT_BUTTON ) {
    if ( state == GLUT_DOWN ) {
      switch ( mode ) {
        case MODE_VIEW:
          followMouse = true;
          prj.shape.posX = TOSCENEX(x);
          prj.shape.posY = TOSCENEY(y);
          break;
        case MODE_SHAPE:
          prj.shape.posX = TOSCENEX(x);
          prj.shape.posY = TOSCENEY(y);
          break;
      }
    } else {
      switch ( mode ) {
        case MODE_VIEW:
          followMouse = false;
          break;
      }
    }
  }
  
  if ( button == GLUT_MIDDLE_BUTTON && state == GLUT_UP ) {
    if ( fullScreen ) {
      glutReshapeWindow ( winOwidth, winOheight );
    } else {
      winOwidth = windowWidth;
      winOheight = windowHeight;
      glutFullScreen();
    }
    fullScreen = ! fullScreen;
  }
  
  mouseX = x;
  mouseY = y;
  
  glutPostRedisplay();
}

void MouseMove ( int x, int y )
{
  int difX = x - mouseX;
  int difY = y - mouseY;
  
  switch ( mode ) {
    case MODE_VIEW:
      if ( followMouse ) {
        prj.shape.posX = TOSCENEX(x);
        prj.shape.posY = TOSCENEY(y);
      }
      mouseX = x;
      mouseY = y;
      break;
    case MODE_COLOR:
    {
      float h = selectedColor->h;
      float s = selectedColor->s;
      float v = selectedColor->v;
      /*unsigned char *sc = selectedColor[0];
       RGBtoHSV ( sc[0], sc[1], sc[2], h, s, v );*/
      
      // Change Color
      if ( shiftOn ) { // change saturation and value
        s += float ( difX ) / 300.0;
        if ( s > 1 ) s = 1;
        if ( s < 0.02 ) s = 0.002;
        v += float ( -difY ) / 300.0;
        if ( v > 1 ) v = 1;
        if ( v < 0.02 ) v = 0.002;
      } else { // change hue
        h += (difX -  difY) / 1.5;
        while ( h < 0 ) h = 360 + h;
        while ( h > 360 ) h = h - 360;
      }
      
      selectedColor->h = h;
      selectedColor->s = s;
      selectedColor->v = v;
      
      //HSVtoRGB ( h, s, v, sc[0], sc[1], sc[2] );
    }
      mouseX = x;
      mouseY = y;
      break;
    case MODE_SHAPE:
      if ( shiftOn ) {
        int dif = ( difX > difY ) ? difX : difY ;
        prj.shape.scaleX = float ( 2 * dif ) / float ( windowWidth );
        prj.shape.scaleY = float ( 2 * dif ) / float ( windowWidth );
      } else {
        prj.shape.scaleX = float ( 2 * difX ) / float ( windowWidth );
        prj.shape.scaleY = float ( 2 * difY ) / float ( windowWidth );
      }
      break;
  }
  
  glutPostRedisplay();
}

void CreateGLWindow( int x, int y, int w, int h )
{
  glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE );
  glutInitWindowPosition( x, y );
  glutInitWindowSize( w, h );
  mainWindow = glutCreateWindow( "Albers Color Studies" );
  glutDisplayFunc( Display );
  glutReshapeFunc( Reshape );
  glutKeyboardFunc( Keyboard );
  glutSpecialFunc( Keyboard2 );
  glutMouseFunc( Mouse );
  glutMotionFunc( MouseMove );
  //glutIdleFunc( Idle );
}

void CreateMenu()
{
  int loadmenu = glutCreateMenu( Menu );
  glutAddMenuEntry ( "Project", CMD_LOAD_PROJECT );
  
  int savemenu = glutCreateMenu( Menu );
  glutAddMenuEntry ( "Project", CMD_SAVE_PROJECT );
  
  /*  int exportmenu = glutCreateMenu( Menu );
   for ( int i=0; i<numPlugins; i++ ) {
   glutAddMenuEntry ( PluginList[i]->filename, CMD_EXPORT_IMAGE + i );
   }
   */
  
  int filemenu = glutCreateMenu( Menu );
  //  glutAddSubMenu ( "Load", loadmenu );
  glutAddMenuEntry ( "Load Project", CMD_LOAD_PROJECT );
  //  glutAddSubMenu ( "Save", savemenu );
  glutAddMenuEntry ( "Save Project", CMD_SAVE_PROJECT );
  //glutAddSubMenu ( "Export Image", exportmenu );
  glutAddMenuEntry ( "Save Image", CMD_SAVE_IMAGE );
  
  int viewmenu = glutCreateMenu( Menu );
  glutAddMenuEntry ( "[H] Show/Hide Help", CMD_HELP );
  glutAddMenuEntry ( "[C] Show/Hide Color Info", CMD_COLORS );
  glutAddMenuEntry ( "[M] Show/Hide Mirror Shape", CMD_MIRROR );
  glutAddMenuEntry ( "[F] Switch between 3/4 color problems", CMD_4COLORS );
  
  int modemenu = glutCreateMenu( Menu );
  glutAddMenuEntry ( "[F1] Edit Colors Mode", MODE_COLOR );
  glutAddMenuEntry ( "[F2] View Mode", MODE_VIEW );
  glutAddMenuEntry ( "[F3] Edit Shape Mode", MODE_SHAPE );
  
  viewmodemenu = glutCreateMenu(Menu);
  glutAddMenuEntry( "", CMD_PLAY_ANIMATION);
  
  colormodemenu = glutCreateMenu ( Menu );
  glutAddMenuEntry ( "[1] Edit Left Color", CMD_EDIT_LEFT_COLOR );
  glutAddMenuEntry ( "[2] Edit Right Color", CMD_EDIT_RIGHT_COLOR );
  glutAddMenuEntry ( "[3] Edit Shape Color", CMD_EDIT_SHAPE_COLOR );
  glutAddMenuEntry ( "[4] Edit Left Shape Color", CMD_EDIT_LEFT_SHAPE_COLOR );
  
  shapemodemenu = glutCreateMenu ( Menu );
  glutAddMenuEntry ( "Rectangle", CMD_SHAPE_RECTANGLE );
  glutAddMenuEntry ( "Circle", CMD_SHAPE_CIRCLE );
  //glutAddMenuEntry ( "Polygon", CMD_SHAPE_POLYGON );
  
  menu = glutCreateMenu( Menu );
  glutAddSubMenu ( "File", filemenu );
  glutAddSubMenu ( "View", viewmenu );
  glutAddSubMenu ( "Mode", modemenu );
  glutAddSubMenu ( "View Mode Options", viewmodemenu );
  glutAddMenuEntry ( "[ESC] Exit", CMD_EXIT );
  glutAttachMenu ( GLUT_RIGHT_BUTTON );
  
  Menu( mode );
}

int main(int argc, char **argv)
{
  glutInit ( &argc, argv );
  CreateGLWindow( 10, 10, windowWidth, windowHeight );
  CreateMenu();
  
  gettimeofday ( &lastDisplayTime, NULL );
  glutMainLoop();
  
  return (0);
}

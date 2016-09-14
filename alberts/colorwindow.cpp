// colorwindow.cpp
// Cem Yuksel

#include "colorwindow.h"
#include "math.h"

#define min(x1,x2)  ((x1)<(x2)?(x1):(x2))
#define max(x1,x2)  ((x1)>(x2)?(x1):(x2))

/*
int colorWindow;

#define CW_WINDOW_WIDTH 	400
#define CW_WINDOW_HEIGHT 	400
#define CW_IMAGE_WIDTH		360
#define CW_IMAGE_HEIGHT		256

unsigned char cwImage [ CW_IMAGE_WIDTH * CW_IMAGE_HEIGHT * 4 ];
*/

void RGBtoHSV (unsigned char r, unsigned char g, unsigned char b, float &h, float &s, float &v) {

	float red, green, blue;
	float maxc, minc, delta;

	// r, g, b to 0 - 1 scale
	red = r / 255.0; green = g / 255.0; blue = b / 255.0;  

	maxc = max(max(red, green), blue);
	minc = min(min(red, green), blue);

	v = maxc;        // value is maximum of r, g, b

	if(maxc == 0){    // saturation and hue 0 if value is 0
		s = 0;
		h = 0;
	} else {
		s = (maxc - minc) / maxc; 	// saturation is color purity on scale 0 - 1

		delta = maxc - minc;
		if(delta == 0)           // hue doesn't matter if saturation is 0 
			h = 0;
		else{
			if(red == maxc)       // otherwise, determine hue on scale 0 - 360
				h = (green - blue) / delta;
			else if(green == maxc)
				h = 2.0 + (blue - red) / delta;
			else // (blue == maxc)
				h = 4.0 + (red - green) / delta;
			h = h * 60.0;
			if(h < 0)
				h = h + 360.0;
		}
	}
}

void HSVtoRGB ( float h, float s, float v, unsigned char &r, unsigned char &g, unsigned char &b )
{
	int i;
	float f, p, q, t, red, green, blue;

	if (s == 0) {
		// grey
		red = green = blue = v;
	} else {
		h /= 60.0;
		i = (int) floor(h);
		f = h - i;
		p = v * (1-s);
		q = v * (1-s*f);
		t = v * (1 - s * (1 - f));

		switch (i) {
			case 0:
				red = v;
				green = t;
				blue = p;
				break;
			case 1:
				red = q;
				green = v;
				blue = p;
				break;
			case 2:
				red = p;
				green = v;
				blue = t;
				break;
			case 3:
				red = p;
				green = q;
				blue = v;
				break;
			case 4:
				red = t;
				green = p;
				blue = v;
				break;
			default:
				red = v;
				green = p;
				blue = q;
				break;
		}
	}

	r = (unsigned char) (red*255.0 + .5);
	g = (unsigned char) (green*255.0 + .5);
	b = (unsigned char) (blue*255.0 + .5);
}

/*void RGBtoHSV ( unsigned char r, unsigned char g, unsigned char b, float &h, float &s, float &v )
{
	unsigned char max = r;
	unsigned char min = r;
	int maxColor = 0;
	
	if ( max < g ) { max = g; maxColor = 1; }
	if ( max < b ) { max = b; maxColor = 2; }
	if ( min > g ) { min = g; }
	if ( min > b ) { min = b; }
	switch ( maxColor ) {
	case 0: // Red
		h = float ( 60*( g - b ) ) / float ( max - min );
		break;
	case 1: // Green
		h = float ( 60*( b - r ) ) / float ( max - min ) + 120.0;
		break;
	case 2: // Blue
		h = float ( 60*( r - g ) ) / float ( max - min ) + 240.0;
		break;
	}
	s = float( max - min ) / (float) max;
	v = (float) max / 255.0;

	while ( h < 0 ) h = 360 + h;
	while ( h > 360 ) h = h - 360;
}

void HSVtoRGB ( float h, float s, float v, unsigned char &r, unsigned char &g, unsigned char &b )
{
	int hi = (int) ( h / 60 );
	float f = h / 60.0  - (float) hi;
	float p = v * ( 1.0 - s );
	float q = v * ( 1.0 - s*f );
	float t = v * ( 1.0 - s*(1.0 - f) );
	switch ( hi ) {
	case 0:
		r = (int) ( v*255.0 );
		g = (int) ( t*255.0 );
		b = (int) ( p*255.0 );
		break;
	case 1:
		r = (int) ( q*255.0 );
		g = (int) ( v*255.0 );
		b = (int) ( p*255.0 );
		break;
	case 2:
		r = (int) ( p*255.0 );
		g = (int) ( v*255.0 );
		b = (int) ( t*255.0 );
		break;
	case 3:
		r = (int) ( p*255.0 );
		g = (int) ( q*255.0 );
		b = (int) ( v*255.0 );
		break;
	case 4:
		r = (int) ( t*255.0 );
		g = (int) ( p*255.0 );
		b = (int) ( v*255.0 );
		break;
	case 5:
		r = (int) ( v*255.0 );
		g = (int) ( p*255.0 );
		b = (int) ( q*255.0 );
		break;
	}
}*/

/*
void CWInitImage()
{
	int y, x;
	for ( y=0; y < CW_IMAGE_HEIGHT; y++ ) {
		for ( x=0; x < CW_IMAGE_WIDTH; x++ ) {
			int index = ( y * CW_IMAGE_WIDTH + x ) * 4;
			cwImage [ index++ ] = x;
			cwImage [ index++ ] = 0;
			cwImage [ index++ ] = 0;
			cwImage [ index++ ] = 0;
		}
	}
}

void CWRenderText ( char *text )
{
	for ( int i=0; i<256; i++ ) {
		if ( text[i] == '\0' ) break;
		glutBitmapCharacter(   GLUT_BITMAP_HELVETICA_18, text[i] );
	}
}


void CWDisplay()
{
	glClear ( GL_COLOR_BUFFER_BIT );
	
	glPixelZoom (1,-1);
	glRasterPos2i ( 100, 0 );
	glDrawPixels ( CW_IMAGE_WIDTH, CW_IMAGE_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, cwImage );
	printf ( "asdf\n" );
	
	glFlush ();
}

void CWReshape( int w, int h )
{
	glViewport ( 0, 0, w, h );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluOrtho2D ( 0, w, h, 0 );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	if ( w != CW_WINDOW_WIDTH || h != CW_WINDOW_HEIGHT ) {
		glutReshapeWindow ( CW_WINDOW_WIDTH, CW_WINDOW_HEIGHT );
	} else {
		glutPostRedisplay();
	}
}

void CWInit()
{
	glClearColor ( 0,0,0,0 );
	CWInitImage();
}

int CreateColorWindow ( int parentwindow, int x, int y )
{
	glutInitDisplayMode( GLUT_RGBA );
	glutInitWindowPosition( x, y );
	glutInitWindowSize( CW_WINDOW_WIDTH, CW_WINDOW_HEIGHT );
	colorWindow = glutCreateWindow( "Cem - Albers Color Studies - 2004" );
	glutDisplayFunc( CWDisplay );
	glutReshapeFunc( CWReshape );
	//glutKeyboardFunc( Keyboard );
	//glutSpecialFunc( Keyboard2 );
	//glutMouseFunc( Mouse );
	//glutMotionFunc( MouseMove );

	glutSetWindow ( parentwindow );
	
	CWInit ();
	
	return colorWindow;
}
*/

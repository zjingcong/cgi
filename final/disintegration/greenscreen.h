/*
green screen and composition functions
*/

void alphamask(int xres, int yres, unsigned char inputpixmap[], unsigned char outputpixmap[]);
void associatedColor(const unsigned char *pixmap, unsigned char *associatedpixmap, int w, int h);
void compose(unsigned char *composedpixmap, unsigned char *frontpixmap, unsigned char *backpixmap, int posX, int posY, int xres, int yres);

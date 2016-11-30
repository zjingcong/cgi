# include "matrix.h"
# include "time.h"

class pieceXform
{
public:
  int pieceid;  // piece id
  int pieceid_x;
  int pieceid_y;
  int pxres; // piece width
  int pyres; // piece height
  int px; // left bottom corner point x
  int py; // left bottom corner point y
  int life_time;
  int life_start_time;

  pieceXform()  {srand(time(NULL));};
  pieceXform(int id, int id_x, int id_y, int a, int b, int c, int d, int e);

  double v_x, v_y, perspective_distance;

  void pieceXformInit(int id, int id_x, int id_y, int a, int b, int c, int d, int e);
  void setLifetime(int t);
  void setStarttime(int t);
  void xformSetting(double vx, double vy, double distance);
  void makehole(unsigned char *outputpixmap, int pic_xres);
  void piecemotion(unsigned char *inputpixmap, unsigned char *outputpixmap, int pic_xres, int pic_yres);
  void pieceUpdate();

private:
  Vector2D xycorners[4];  // output piece corner positions
  Vector2D inputxycorners[4]; // input piece corner positions: 0 - left bottom, 1 - left up, 2 - right up, 3 - right bottom
  Matrix3D transMatrix;  // transform matrix for the piece
  Matrix3D transMatrix_tmp;

  int out_pxres;  // output piece width
  int out_pyres;  //output piece height
  int out_px; // output left bottom corner point x
  int out_py; // output left bottom corner point y

  void getinputcorners(); // get input corners positions
  void boundingbox(); // get output corners positions, output width and output height
  void generateMatrix(char tag, double px, double py);
  // void makehole(unsigned char *outputpixmap, int pic_xres);
  void inversemap(unsigned char *inputpixmap, unsigned char *outputpixmap, int pic_xres, int pic_yres);
};

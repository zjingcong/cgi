/*
   Matrix manipulation routines

   D. H. House  10/26/94
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

#include "matrix.h"

using namespace std;

Matrix3D::Matrix3D(){
  setidentity();
}

Matrix3D::Matrix3D(const double coefs[3][3]){
  set(coefs);
}

Matrix3D::Matrix3D(const Matrix3D &mat){
  set(mat.M);
}

/*
   Print the contents of a 3x3 transformation matrix
*/
void Matrix3D::print()const{

  printf("\n");
  for(int row = 0; row < 3; row++){
    printf("%8.4f %8.4f %8.4f\n", M[row][0], M[row][1], M[row][2]);
  }
  printf("\n");
}

/*
   Set the matrix m to the indentity matrix
*/
void Matrix3D::setidentity(){

  M[0][0] = M[1][1] = M[2][2] = 1.0;
  M[0][1] = M[0][2] = 0.0;
  M[1][0] = M[1][2] = 0.0;
  M[2][0] = M[2][1] = 0.0;
}

/*
 Set the matrix m to the contents of a 3x3 array
 */
void Matrix3D::set(const double coefs[3][3]){

  for(int row = 0; row < 3; row++)
	for(int col = 0; col < 3; col++)
	  M[row][col] = coefs[row][col];
}

/*
   Function to compute and return the determinant of the 3x3 matrix m.
*/
double Matrix3D::determinant()const{
  double det;

  det  = M[0][0] * M[1][1] * M[2][2];
  det += M[0][1] * M[1][2] * M[2][0];
  det += M[0][2] * M[2][1] * M[1][0];
  det -= M[2][0] * M[1][1] * M[0][2];
  det -= M[1][0] * M[0][1] * M[2][2];
  det -= M[0][0] * M[1][2] * M[2][1];

  return det;
}

/*
   Compute the adjoint of 3x3 matrix m and place the result in 3x3
   matrix adj.
*/
Matrix3D Matrix3D::adjoint()const{
  Matrix3D adj;

  adj.M[0][0] = (M[1][1] * M[2][2] - M[1][2] * M[2][1]);
  adj.M[0][1] = (M[0][2] * M[2][1] - M[0][1] * M[2][2]);
  adj.M[0][2] = (M[0][1] * M[1][2] - M[0][2] * M[1][1]);
  adj.M[1][0] = (M[1][2] * M[2][0] - M[1][0] * M[2][2]);
  adj.M[1][1] = (M[0][0] * M[2][2] - M[0][2] * M[2][0]);
  adj.M[1][2] = (M[0][2] * M[1][0] - M[0][0] * M[1][2]);
  adj.M[2][0] = (M[1][0] * M[2][1] - M[1][1] * M[2][0]);
  adj.M[2][1] = (M[0][1] * M[2][0] - M[0][0] * M[2][1]);
  adj.M[2][2] = (M[0][0] * M[1][1] - M[0][1] * M[1][0]);

  return adj;
}

/*
   Compute the inverse of 3x3 matrix m and place the result in 3x3
   matrix inv.
*/
Matrix3D Matrix3D::inverse()const{
  Matrix3D inv;
  double det = determinant();

  inv.M[0][0] = (M[1][1] * M[2][2] - M[1][2] * M[2][1]) / det;
  inv.M[0][1] = (M[0][2] * M[2][1] - M[0][1] * M[2][2]) / det;
  inv.M[0][2] = (M[0][1] * M[1][2] - M[0][2] * M[1][1]) / det;
  inv.M[1][0] = (M[1][2] * M[2][0] - M[1][0] * M[2][2]) / det;
  inv.M[1][1] = (M[0][0] * M[2][2] - M[0][2] * M[2][0]) / det;
  inv.M[1][2] = (M[0][2] * M[1][0] - M[0][0] * M[1][2]) / det;
  inv.M[2][0] = (M[1][0] * M[2][1] - M[1][1] * M[2][0]) / det;
  inv.M[2][1] = (M[0][1] * M[2][0] - M[0][0] * M[2][1]) / det;
  inv.M[2][2] = (M[0][0] * M[1][1] - M[0][1] * M[1][0]) / det;

  return inv;
}

/*
 Transform the 2D vector v by by the 3x3 matrix m.  The process is to
 1) extend v by adding a w coordinate of 1.0,
 2) premultiply this 3D vector by m,
 3) scale the resulting vector by 1/w,
 4) Place the x and y coordinates of the result in vout.
 */
Vector2D Matrix3D::operator*(const Vector2D &v)const{
  Vector3D vw;
  Vector2D vout;

  vw.x =  M[0][0] * v.x + M[0][1] * v.y + M[0][2];
  vw.y =  M[1][0] * v.x + M[1][1] * v.y + M[1][2];
  vw.w =  M[2][0] * v.x + M[2][1] * v.y + M[2][2];

  if(vw.w == 0.0){
    fprintf(stderr, "WARNING: w coordinate of 0!\n");
    vout.x = vw.x;
    vout.y = vw.y;
  }
  else if(vw.w == 1.0){
    vout.x = vw.x;
    vout.y = vw.y;
  }
  else{
    vout.x = vw.x / vw.w;
    vout.y = vw.y / vw.w;
  }

  return vout;
}

/*
 Multiply 3x3 matrix m1 by 3x3 matrix m2 and place the result in
 3x3 matrix m3.  I.e. m3 = m1 * m2
 */
Matrix3D Matrix3D::operator*(const Matrix3D &m2)const{
  double sum;
  int row, col, rc;
  Matrix3D prod;	/* make internal copy in case m3 is m1 or m2 */

  for(row = 0; row < 3; row++)
    for(col = 0; col < 3; col++){
      sum = 0.0;
      for(rc = 0; rc < 3; rc++)
		sum += M[row][rc] * m2.M[rc][col];
      prod.M[row][col] = sum;
    }

  return prod;
}

double *Matrix3D::operator[](int i){
  return (double *)&(M[i]);
}

void setbilinear(double width, double height, Vector2D xycorners[4],
		 BilinearCoeffs &coeff){

  coeff.width = width;
  coeff.height = height;
  coeff.a0 = xycorners[0].x;
  coeff.b0 = xycorners[0].y;
  coeff.a1 = xycorners[3].x - xycorners[0].x;
  coeff.b1 = xycorners[3].y - xycorners[0].y;
  coeff.a2 = xycorners[1].x - xycorners[0].x;
  coeff.b2 = xycorners[1].y - xycorners[0].y;
  coeff.a3 = xycorners[2].x - xycorners[1].x -
    xycorners[3].x + xycorners[0].x;
  coeff.b3 = xycorners[2].y - xycorners[1].y -
    xycorners[3].y + xycorners[0].y;
  coeff.c2 = coeff.a3 * coeff.b2 - coeff.a2 * coeff.b3;
  // cout << coeff.a0 << " " << coeff.b0 << " " << coeff.a1 << " " << coeff.b1 << " " << coeff.a2 << " " << coeff.b2 << endl;
}

/*
   Compute the inverse bilinear transform of the point xy, back
   into the unit normalized source space uv, using the coefficient block c.
   Then, the uv coordinates unnormalized back into the source image space.
*/
void invbilinear(const BilinearCoeffs &c, Vector2D xy, Vector2D &uv){
  double discriminant;
  double c0, c1;
  static double EPSILON = 1.0e-5;

  // cout << c.a0 << " " << c.b0 << " " << c.a1 << " " << c.b1 << " " << c.a2 << " " << c.b2 << endl;

  if(fabs(c.c2) <= EPSILON){
    uv.y = (c.a1 * xy.y - c.a1 * c.b0 - c.b1 * xy.x + c.a0 * c.b1) /
      (c.a1 * c.b2 - c.a2 * c.b1);
    uv.x = (xy.x - c.a0 - c.a2 * uv.y) / c.a1;
  }
  else{
    c0 = c.a1 * (c.b0 - xy.y) + c.b1 * (xy.x - c.a0);
    c1 = c.a3 * (c.b0 - xy.y) + c.b3 * (xy.x - c.a0) +
      c.a1 * c.b2 - c.a2 * c.b1;

    discriminant = sqrt(c1 * c1 - 4.0 * c.c2 * c0);

    uv.y =(-c1 + discriminant) / (2.0 * c.c2);
    if(uv.y < 0.0 || uv.y > 1.0){
      uv.y =(-c1 - discriminant) / (2.0 * c.c2);
      uv.x =
	(xy.x - c.a0 - c.a2 * uv.y) / (c.a1 + c.a3 * uv.y);
    }
    else{
      uv.x =
	(xy.x - c.a0 - c.a2 * uv.y) / (c.a1 + c.a3 * uv.y);
      if(uv.x < 0.0 || uv.x > 1.0){
	uv.y =(-c1 - discriminant) / (2.0 * c.c2);
	uv.x =
	  (xy.x - c.a0 - c.a2 * uv.y) / (c.a1 + c.a3 * uv.y);
      }
    }
  }

  uv.x *= c.width;
  uv.y *= c.height;
}

/*
   Definitions for Matrix manipulation routines

   D. H. House  10/26/94
*/

#include <cstdio>
#include <cmath>

#ifndef PI
#define PI		3.1415926536
#endif

struct Vector3D{
  double x, y, w;
};

struct Vector2D{
  double x, y;
};

class Matrix3D{
private:
  double M[3][3];

public:
  Matrix3D();
  Matrix3D(const double coefs[3][3]);
  Matrix3D(const Matrix3D &mat);

  void print() const;

  void setidentity();
  void set(const double coefs[3][3]);

  double determinant() const;
  Matrix3D adjoint() const;
  Matrix3D inverse() const;

  Vector2D operator*(const Vector2D &v) const;
  Matrix3D operator*(const Matrix3D &m2) const;
  double *operator[](int i);
};

struct BilinearCoeffs{
  double width, height;
  double a0, a1, a2, a3;
  double b0, b1, b2, b3;
  double c2;
};

void setbilinear(double width, double height,
		 Vector2D xycorners[4], BilinearCoeffs &coeff);
void invbilinear(const BilinearCoeffs &c, Vector2D xy, Vector2D &uv);

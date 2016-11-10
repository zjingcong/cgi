/*
  Standard C++ version of an example warp

  Example inverse warp designed by Donald House
  CPSC 4040/6040, 10/18/16
*/

# include <cmath>

# ifndef PI
# define PI 3.1415926536
# endif

/*
  Routine to inverse map (x, y) output image spatial coordinates
  into (u, v) input image spatial coordinates

  Call routine with (x, y) spatial coordinates in the output
  image. Returns (u, v) spatial coordinates in the input image,
  after applying the inverse map. Note: (u, v) and (x, y) are not 
  rounded to integers, since they are true spatial coordinates.
 
  inwidth and inheight are the input image dimensions
  outwidth and outheight are the output image dimensions
*/
inv_map(float x, float y, float &u, float &v,
	int inwidth, int inheight, int outwidth, int outheight){
  
  x /= outwidth;		// normalize (x, y) to (0...1, 0...1)
  y /= outheight;

  u = sqrt(x);			        // inverse in x direction is sqrt
  v = 0.5 * (1 + sin(y * PI));  // inverse in y direction is offset sine

  u *= inwidth;			// scale normalized (u, v) to pixel coords
  v *= inheight;

}


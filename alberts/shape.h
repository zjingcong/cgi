// shape.h

#ifndef _SHAPE_INCLUDED_
#define _SHAPE_INCLUDED_

#include <math.h>
#include <stdio.h>

#define NUM_CIRCLE_SEGMENTS 24

class Vertex 
{
public:
	Vertex ( Vertex *n, float x, float y ) : Next(n) { v[0]=x; v[1]=y; }
	virtual ~Vertex () { if ( Next ) delete Next; }
	float v[2];
	Vertex *Next;
};

class Shape
{
public:
	Shape () : verts(NULL), scaleX(0.1), scaleY(0.1), posX(0), posY(0) { SetRectangle(); }
	Vertex* GetVerts() { return verts; }
	void AddVertex ( float x, float y ) { verts = new Vertex ( verts, x, y ); }
	void ClearVertices () { if (verts) delete verts; verts = NULL; }
	void SetRectangle () { ClearVertices(); AddVertex(-1,-1); AddVertex(1,-1); AddVertex(1,1); AddVertex(-1,1); }
	void SetCircle () {
		ClearVertices();
		float x = 1, y =0;
		float angle = 2.0 * M_PI / NUM_CIRCLE_SEGMENTS;
		float c = cos ( angle );
		float s = sin ( angle );
		for ( int i=0; i < NUM_CIRCLE_SEGMENTS; i++ ) {
			AddVertex ( x, y );
			printf ( "(%f, %f)\n", x, y );
			// (nx,ny) = cos*(x,y) + sin*(y,-x)
			float nx = c*x + s*y;
			float ny = c*y - s*x;
			x = nx; y = ny;
		}
		AddVertex ( 1, 0 );
	}

	float scaleX, scaleY;
	float posX, posY;

private:
	Vertex *verts;
};


#endif

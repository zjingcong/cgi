// project.h
// Cem Yuksel

#ifndef _PROJECT_INCLUDED_
#define _PROJECT_INCLUDED_


#include "shape.h"
#include "record.h"
#include "colorwindow.h"

//typedef unsigned char Color3[3];

struct HSV_Color {
	float h, s, v;
};

class Project
{
public:
	Project();
	virtual ~Project();

	int Save ( char *filename );
	int Load ( char *filename );
	
	Shape rightShape;
    Shape leftShape;
	HSV_Color leftColor, rightColor, rightShapeColor, leftShapeColor;
	Record record;
	bool displayMirror;
    bool is4colorproblem;

private:
	void SaveColors ( FILE *fp );
	void LoadColors ( FILE *fp );
	void SaveShape  ( FILE *fp );
	void LoadShape  ( FILE *fp );

};

#endif

// project.cpp
// Cem Yuksel

#include <stdlib.h>
#include <stdio.h>

#include "project.h"


Project::Project()
{
	/*leftColor[0] = 200;
	leftColor[1] = 0;
	leftColor[2] = 12;
	rightColor[0] = 160;
	rightColor[1] = 55;
	rightColor[2] = 0;
	shapeColor[0] = 184;
	shapeColor[1] = 33;
	shapeColor[2] = 0;*/
	leftColor.h = 356.0;
	leftColor.s = 1.0;
	leftColor.v = 0.78;
	rightColor.h = 21.0;
	rightColor.s = 1.0;
	rightColor.v = 0.63;
    
    //right shape
	rightShapeColor.h = 11.0;
	rightShapeColor.s = 1.0;
	rightShapeColor.v = 0.72;
	
	rightShape.posX = 0.5;
	rightShape.posY = 0.0;

    //left shape
    leftShapeColor.h = 79.0;
    leftShapeColor.s = 1.0;
    leftShapeColor.v = 0.72;

    leftShape.posX = -0.5;
    leftShape.posY = 0.0;
	
	displayMirror = false;
    is4colorproblem = true;
}

Project::~Project()
{
}


int Project::Save( char *filename )
{
	FILE *fp;
	fp = fopen ( filename, "w" );

	if ( fp ) {	
		// Save that this is project file
		fprintf ( fp, "Project\n" );

		// Save Display Mirror
		fprintf ( fp, "%d\n", (int)displayMirror );
	
		SaveColors ( fp );
		SaveShape ( fp );
	
		fclose ( fp );
	
		return 1;
	} else {
		return 0;
	}
}

int Project::Load( char *filename )
{
	char buffer[256];
	FILE *fp;
	fp = fopen ( filename, "r" );
	
	if ( fp ) {
		// Load file info
		fscanf ( fp, "%s", buffer );
	
		if ( buffer[0] != 'P' ) {
			fclose ( fp );
			return 0;
		}

		// Load Display Mirror
		int dm;
		fscanf ( fp, "%d", &dm );
		displayMirror = dm;
	
		LoadColors ( fp );	
		LoadShape ( fp );
	
		fclose ( fp );
	
		return 1;
	} else {
		return 0;
	}
}


//******************************************************************************
// PRIVATE FUNCTIONS
//******************************************************************************


void Project::SaveColors ( FILE *fp )
{
	unsigned char r, g, b;
	HSVtoRGB(leftColor.h, leftColor.s, leftColor.v, r, g, b);
	fprintf ( fp, "%d %d %d\n", r, g, b );
	HSVtoRGB(rightColor.h, rightColor.s, rightColor.v, r, g, b);
	fprintf ( fp, "%d %d %d\n", r, g, b );
	HSVtoRGB(rightShapeColor.h, rightShapeColor.s, rightShapeColor.v, r, g, b);
	fprintf ( fp, "%d %d %d\n", r, g, b );
    HSVtoRGB(leftShapeColor.h, leftShapeColor.s, leftShapeColor.v, r, g, b);
	fprintf ( fp, "%d %d %d\n", r, g, b );
}

void Project::LoadColors ( FILE *fp )
{
	int r, g, b;
	fscanf ( fp, "%d %d %d", &r, &g, &b );
	RGBtoHSV(r, g, b, leftColor.h, leftColor.s, leftColor.v);
	fscanf ( fp, "%d %d %d", &r, &g, &b );
	RGBtoHSV(r, g, b, rightColor.h, rightColor.s, rightColor.v);
	fscanf ( fp, "%d %d %d", &r, &g, &b );
	RGBtoHSV(r, g, b, rightShapeColor.h, rightShapeColor.s, rightShapeColor.v);
    fscanf ( fp, "%d %d %d", &r, &g, &b );
	RGBtoHSV(r, g, b, leftShapeColor.h, leftShapeColor.s, leftShapeColor.v);
}

void Project::SaveShape  ( FILE *fp )
{
	fprintf ( fp, "%f %f\n", rightShape.posX, rightShape.posY );
	fprintf ( fp, "%f %f\n", rightShape.scaleX, rightShape.scaleY );
	Vertex *v = rightShape.GetVerts ();
	while ( v != NULL ) {
		fprintf ( fp, "%f %f\n", v->v[0], v->v[1] );
		v = v->Next;
	}
	fprintf ( fp, "9999 9999\n" );
}

void Project::LoadShape  ( FILE *fp )
{
	rightShape.ClearVertices();
	fscanf ( fp, "%f %f", &rightShape.posX, &rightShape.posY );
	fscanf ( fp, "%f %f", &rightShape.scaleX, &rightShape.scaleY );
	float x, y;
	fscanf ( fp, "%f %f", &x, &y );
	while ( x < 999 ) {
		rightShape.AddVertex ( x, y );
		fscanf ( fp, "%f %f", &x, &y );
	}
}

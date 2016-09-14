// record.cpp
// Cem Yuksel

#include "record.h"

Record::Record()
{
	first = NULL;
	last = NULL;
}

Record::~Record()
{
	Clear();
}

void Record::Clear()
{
	while ( first != NULL ) {
		RecordKey *delkey = first;
		first = delkey->next;
		delete delkey;
	}
	first = NULL;
	last = NULL;
}

void Record::AddKey( int time, float x, float y )
{
	RecordKey *newkey = new RecordKey;
	newkey->time = time;
	newkey->x = x;
	newkey->y = y;
	newkey->next = NULL;
	
	if ( last ) {	// if there is a last key
		last->next = newkey;
	} else {	// if there is no last key (meaning, there are no keys!)
		first = newkey;
	}
	last = newkey;
}

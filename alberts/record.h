// record.h
// Cem Yuksel

#ifndef _RECORD_INCLUDED_
#define _RECORD_INCLUDED_

#include <stdlib.h>
#include <stdio.h>

struct RecordKey
{
	int time;
	float x,y;
	RecordKey *next;
};

class Record
{
public:
	Record ();
	virtual ~Record();

	void Clear ();
	void AddKey ( int time, float x, float y );
	RecordKey* GetFirstKey() { return first; }
	RecordKey* GetLastKey() { return last; }

private:
	RecordKey *first;
	RecordKey *last;
};

#endif

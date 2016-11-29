
CC		= g++
C		= cpp

CFLAGS		= -g -Wall `Magick++-config --cppflags`
LFLAGS		= -g `Magick++-config --ldflags`

ifeq ("$(shell uname)", "Darwin")
  LDFLAGS     = -framework Foundation -framework GLUT -framework OpenGL -lMagick++ -lm
else
  ifeq ("$(shell uname)", "Linux")
    LDFLAGS     = -L /usr/lib64/ -lglut -lGL -lMagick++ -lGLU -lOpenImageIO -lm
  endif
endif

HFILES	= greenscreen.h disolvefx.h matrix.h
OFILES  = greenscreen.o disolvefx.o matrix.o

PROJECT		= disintegration

${PROJECT}:	${PROJECT}.o ${OFILES}
	${CC} ${LFLAGS} -o ${PROJECT} ${PROJECT}.o ${OFILES} ${LDFLAGS}

${PROJECT}.o:	${PROJECT}.${C} ${HFILES}
	${CC} ${CFLAGS} -c ${PROJECT}.${C}

matrix.o: matrix.${C} matrix.h
	${CC} ${CFLAGS} -c matrix.${C}

greenscreen.o: greenscreen.${C} greenscreen.h
	${CC} ${CFLAGS} -c greenscreen.${C}

disolvefx.o: disolvefx.${C} disolvefx.h matrix.h
	${CC} ${CFLAGS} -c disolvefx.${C}

clean:
	rm -f core.* *.o *~ ${PROJECT}

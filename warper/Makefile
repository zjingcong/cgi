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

HFILES	= matrix.h
OFILES  = matrix.o

PROJECT		= warper

${PROJECT}:	${PROJECT}.o ${OFILES}
	${CC} ${LFLAGS} -o ${PROJECT} ${PROJECT}.o ${OFILES} ${LDFLAGS}
	
${PROJECT}.o:	${PROJECT}.${C} ${HFILES}
	${CC} ${CFLAGS} -c ${PROJECT}.${C}
	
clean:
	rm -f core.* *.o *~ ${PROJECT}

CC		= g++
C		= cpp

CFLAGS		= -g
LFLAGS		= -g

ifeq ("$(shell uname)", "Darwin")
  LDFLAGS     = -framework Foundation -framework GLUT -framework OpenGL -lOpenImageIO -lm
else
  ifeq ("$(shell uname)", "Linux")
    LDFLAGS   = -L /usr/lib64/ -lglut -lGL -lGLU -lOpenImageIO -lm
  endif
endif

PROJECT1		= wrap
PROJECT2		= tile

all: ${PROJECT1} ${PROJECT2}

${PROJECT1}:	${PROJECT1}.o
	${CC} ${LFLAGS} -o ${PROJECT1} ${PROJECT1}.o ${LDFLAGS}

${PROJECT1}.o:	${PROJECT1}.${C}
	${CC} ${CFLAGS} -c ${PROJECT1}.${C}

${PROJECT2}:  ${PROJECT2}.o
	${CC} ${LFLAGS} -o ${PROJECT2} ${PROJECT2}.o ${LDFLAGS}

${PROJECT2}.o:  ${PROJECT2}.${C}
	${CC} ${CFLAGS} -c ${PROJECT2}.${C}

clean:
	rm -f core.* *.o *~ ${PROJECT1}
	rm -f core.* *.o *~ ${PROJECT2} 

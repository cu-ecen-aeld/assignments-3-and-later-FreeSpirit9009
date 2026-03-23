# requirements
# a. Is compiled by the “all” and “default” target of a Makefile in the “server” directory and supports cross compilation, placing the executable file in the “server” directory and named aesdsocket.

# targets
default: all

# sources, objects, executables
SOURCES = aesdsocket.c

# Below is a text substitution, see http://sunsite.ualberta.ca/Documentation/Gnu/make-3.79/html_chapter/make_8.html
OBJS    = $(SOURCES:.c=.o)
EXE     = aesdsocket

all: $(EXE)

$(EXE): $(OBJS)
	gcc $(OBJS) -Wall -Wextra -Werror -o $(EXE)

# Below is an inference rule, that is, a rule that generically applies to multiple targets/rules. See http://www-cip.physik.uni-bonn.de/pool/infos/make/advanced.html
%.o: %.c
	gcc -Wall -Wextra -Werror -g -c $*.c

clean:
	rm -rf *.o
	rm -rf $(EXE)

test:
	echo SOURCES = $(SOURCES)
	echo OBJS = $(OBJS)
	echo EXE = $(EXE)


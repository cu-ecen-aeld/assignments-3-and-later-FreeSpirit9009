#
# Makefile for writer.c
#
# Requirements:
# - A default target which builds the “writer” application
# - A clean target which removes the “writer” application and all .o files
# - Support for cross-compilation.  You should be able to generate an application for the native build platform when GNU make variable CROSS_COMPILE is not specified on the make command line.  When CROSS_COMPILE is specified with aarch64-none-linux-gnu- (note the trailing -)your makefile should compile successfully using the cross compiler installed in step 1.

# Settings: determine the compiler. We provide the CROSSPACKAGE and allow user to see it by using make debug,
# so if the compiler is missing, they know what to install
ifeq ($(CROSS_COMPILE), aarch64-none-linux-gnu-)
else ifeq ($(CROSS_COMPILE), "")
else
$(error: Invalid value of CROSS_COMPILE: $(CROSS_COMPILE))
CROSS_COMPILE = 
endif
CC = $(CROSS_COMPILE)gcc

# sources
NAME = writer
SOURCE = $(NAME).c

# default target
EXECUTABLE = $(NAME)
default: $(EXECUTABLE)

$(EXECUTABLE): $(SOURCE)
	$(CC) $(SOURCE) -o $(EXECUTABLE)

.PHONY:
clean:
	rm -f *.o $(EXECUTABLE)

.PHONY:
debug:
	echo $(CROSS_COMPILE)


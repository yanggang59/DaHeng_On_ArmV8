# Makefile for sample program
.PHONY			: all clean

# the program to build
NAME			:= main

# Build tools and flags
CXX			:= g++
LD			:= g++
SRCS                    := $(wildcard *.cpp)
OBJS                    := $(patsubst %cpp, %o, $(SRCS))
CPPFLAGS		:= -w -I/opt/ros/kinetic/include/opencv-3.3.1-dev/ \
			       -I./
ifeq ($(shell getconf LONG_BIT),64)
LDFLAGS			:=-lgxiapi -lpthread \
-L/opt/ros/kinetic/lib/aarch64-linux-gnu \
-lopencv_calib3d3 -lopencv_core3 -lopencv_dnn3 -lopencv_features2d3 -lopencv_flann3 -lopencv_highgui3 -lopencv_imgcodecs3 -lopencv_imgproc3 -lopencv_ml3 -lopencv_objdetect3 -lopencv_shape3 -lopencv_stitching3 -lopencv_superres3 -lopencv_video3 -lopencv_videoio3 -lopencv_videostab3
else
LDFLAGS                 :=-lgxiapi -lpthread
endif

all			: $(NAME)

$(NAME)			: $(OBJS)
	$(LD) -o $@ $^ $(CPPFLAGS) $(LDFLAGS)

%.o		: %.cpp
	$(CXX) $(CPPFLAGS) -c -o $@ $<

clean			:
	$(RM) *.o $(NAME)


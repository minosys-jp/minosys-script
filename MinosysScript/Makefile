LIBSRC=lex.cc content.cc engine.cc
LIBOBJ=$(LIBSRC:.cc=.o)
SRC=main.cc
OBJ=$(SRC:.cc=.o)
LIBTARGET=libminosysscr.so
TARGET=minosysscr
CXXFLAGS=-g -O0 -fPIC -std=c++11
LDFLAGS=
LIBS=-L. -lminosysscr -ldl
CXX=g++

all: $(TARGET)

$(TARGET): $(OBJ) $(LIBTARGET)
	$(CXX) $(LDFLAGS) -o $(TARGET) $(OBJ) $(LIBS)

$(LIBTARGET): $(LIBOBJ)
	$(CXX) -shared -o $(LIBTARGET) $(LIBOBJ)

.cc.o:
	$(CXX) -c $(CXXFLAGS) $<

clean:
	-rm $(TARGET) $(OBJ) $(LIBTARGET) $(LIBOBJ)

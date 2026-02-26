MINGW_BIN = C:/Users/root/AppData/Local/Microsoft/WinGet/Packages/BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe/mingw64/bin

CXX      = $(MINGW_BIN)/g++
CXXFLAGS = -O2 -Wall -std=c++17 -DPDC_WIDE -I./PDCurses
LDFLAGS  = -static -L./PDCurses/wincon -l:pdcurses.a -lshell32 -lole32

SRCS = main.cpp ui.cpp datastore.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = eboxlab.exe

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	del /Q *.o $(TARGET) 2>NUL || rm -f *.o $(TARGET)

.PHONY: all clean

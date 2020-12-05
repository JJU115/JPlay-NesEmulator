CXX = g++
DBG = -O3 -Wall
# Update these paths as necessary to match your installation
SDL_LIB = -LC:/SDL/i686-w64-mingw32/lib -lSDL2main -lSDL2
SDL_INCLUDE = -IC:/SDL/i686-w64-mingw32/include/SDL2
# If your compiler is a bit older you may need to change -std=c++11 to -std=c++0x
CXXFLAGS = -O3 -Wall -c -std=c++11 $(SDL_INCLUDE)
LDFLAGS = -lmingw32 -mwindows -mconsole $(SDL_LIB)
FILES = APU.o Display.o CPU.o PPU.o NES.o Cartridge.o NROM.o MMC1.o UxROM.o CxROM.o MMC3.o AxROM.o MMC2.o GxROM.o
EXE = NES.exe

all: $(EXE)


$(EXE): clean Display.o CPU.o PPU.o APU.o NES.o Cartridge.o NROM.o MMC1.o UxROM.o CxROM.o MMC3.o AxROM.o MMC2.o GxROM.o

	$(CXX) $(DBG) $(FILES) $(LDFLAGS) -o $@

NES.o: NES.cpp

	$(CXX) $(CXXFLAGS) $< -o $@

Display.o: Display.cpp

	$(CXX) $(CXXFLAGS) $< -o $@

CPU.o: CPU.cpp

	$(CXX) $(CXXFLAGS) $< -o $@

PPU.o: PPU.cpp

	$(CXX) $(CXXFLAGS) $< -o $@

APU.o: APU.cpp

	$(CXX) $(CXXFLAGS) $< -o $@

Cartridge.o: Cartridge.cpp

	$(CXX) $(CXXFLAGS) $< -o $@

NROM.o: NROM.cpp

	$(CXX) $(CXXFLAGS) $< -o $@

MMC1.o: MMC1.cpp

	$(CXX) $(CXXFLAGS) $< -o $@

UxROM.o: UxROM.cpp

	$(CXX) $(CXXFLAGS) $< -o $@

CxROM.o : CxROM.cpp

	$(CXX) $(CXXFLAGS) $< -o $@

MMC3.o : MMC3.cpp

	$(CXX) $(CXXFLAGS) $< -o $@

AxROM.o : AxROM.cpp

	$(CXX) $(CXXFLAGS) $< -o $@

MMC2.o : MMC2.cpp

	$(CXX) $(CXXFLAGS) $< -o $@

GxROM.o : GxROM.cpp

	$(CXX) $(CXXFLAGS) $< -o $@

clean:

	del *.o && del $(EXE)
CXX = g++
# Update these paths as necessary to match your installation
SDL_LIB = -LC:/SDL/i686-w64-mingw32/lib -lSDL2main -lSDL2
SDL_INCLUDE = -IC:/SDL/i686-w64-mingw32/include
# If your compiler is a bit older you may need to change -std=c++11 to -std=c++0x
CXXFLAGS = -c -std=c++11 $(SDL_INCLUDE)
LDFLAGS = -lmingw32 -mwindows -mconsole $(SDL_LIB)
FILES = NES.o Display.o CPU.o PPU.o Cartridge.o NROM.o MMC1.o
EXE = NES.exe

all: $(EXE)


$(EXE): NES.o Display.o CPU.o PPU.o Cartridge.o NROM.o MMC1.o

	$(CXX) $(FILES) $(LDFLAGS) -o $@

NES.o: NES.cpp

	$(CXX) $(CXXFLAGS) $< -o $@

Display.o: Display.cpp

	$(CXX) $(CXXFLAGS) $< -o $@

CPU.o: CPU.cpp

	$(CXX) $(CXXFLAGS) $< -o $@

PPU.o: PPU.cpp

	$(CXX) $(CXXFLAGS) $< -o $@

Cartridge.o: Cartridge.cpp

	$(CXX) $(CXXFLAGS) $< -o $@

NROM.o: NROM.cpp

	$(CXX) $(CXXFLAGS) $< -o $@

MMC1.o: MMC1.cpp

	$(CXX) $(CXXFLAGS) $< -o $@

clean:

	del *.o && del $(EXE)
# JPlay-NesEmulator
An emulator of the Nintendo Entertainment System for Windows written in C++ as a hobby project.

<img src="/Images/Tetris.PNG?raw=true" width=200 height=200 align="left"> <img src="/Images/Kirby.PNG?raw=true" width=200 height=200 align="left"> <img src="/Images/Gradius.PNG?raw=true" width=200 height=200 align="left"> <img src="/Images/Tyson.PNG?raw=true" width=200 height=200 align="left">
<br>
<img src="/Images/SMB3.PNG?raw=true" width=200 height=200 align="left"> <img src="/Images/Zelda.PNG?raw=true" width=200 height=200 align="left"> <img src="/Images/SMB.PNG?raw=true" width=200 height=200 align="left"> <img src="/Images/Cabal.PNG?raw=true" width=200 height=200>



## Current State
The system is working and can play most games without trouble. The only outstanding issues are:

  * The DMC channel of the APU isn't functional
  * Games that perform mid frame state changes (i.e split the the screen) may appear a little wonky

Aside from that JPlay implements and can play games using the following iNES mappers:

  * (0) NROM
  * (1) MMC1
  * (2) UxROM
  * (3) CxROM
  * (4) MMC3
  * (7) AxROM
  * (9) MMC2
  * (66) GxROM

This amounts to ~1412 different games.

## Dependencies
JPlay makes use of and requires SDL2.0 to run 


## Usage
If you only want to run the program then all you need is the NES.exe file and your .nes file.<br>
```
//The file needs to be a single argument so wrap it in double quotes if it has spaces
$ NES <.nes file>
```

You can compile it yourself with the makefile and mingw32-make on Windows.

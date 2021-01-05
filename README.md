# JPlay-NesEmulator
An emulator of the Nintendo Entertainment System for Windows written in C++ as a hobby project.

<img src="/Images/Tetris.PNG?raw=true" width=200 height=200 align="left"> <img src="/Images/Kirby.PNG?raw=true" width=200 height=200 align="left"> <img src="/Images/Gradius.PNG?raw=true" width=200 height=200 align="left"> <img src="/Images/Tyson.PNG?raw=true" width=200 height=200 align="left">
<br>
<img src="/Images/SMB3.PNG?raw=true" width=200 height=200 align="left"> <img src="/Images/Zelda.PNG?raw=true" width=200 height=200 align="left"> <img src="/Images/SMB.PNG?raw=true" width=200 height=200 align="left"> <img src="/Images/Cabal.PNG?raw=true" width=200 height=200>



## Current State
The system is working and can play most games without trouble. Games which use special
visual effects or mid frame state changes (i.e split the screen) may appear a little wonky.
In terms of faithfulness to original hardware the APU deviates the most, as such the audio
isn't perfect but any problems are likely to be barely noticeable.

  
JPlay implements and can play games using the following iNES mappers:

  * (0) NROM
  * (1) MMC1
  * (2) UxROM
  * (3) CxROM
  * (4) MMC3
  * (7) AxROM
  * (9) MMC2
  * (66) GxROM

This amounts to ~1412 different games. Note that ROM dumps with bad iNES headers will likely crash the program.
Of the first 16 bytes only the first 6 are used, GxROM mapped games will use the top 4 bits of byte 7, the remaining
unused bytes should be 0.

## Dependencies
JPlay requires SDL2.0 to run 


## Usage
If you only want to run the program then all you need is the NES.exe file and your .nes file.<br>
```
//The file needs to be a single argument so wrap it in double quotes if it has spaces
$ NES <.nes file>
```

You can compile it yourself with the makefile and mingw32-make on Windows. Change the SDL_LIB and SDL_INCLUDE variables to match your SDL2 installation. You'll need the SDL2 development libraries --> https://www.libsdl.org/download-2.0.php


## Controller Mapping
Controller input is currently mapped to the following keybindings:

Button | Key
------ | ---
A | A
B | S
Up | Up Arrow
Left | Left Arrow
Right | Right Arrow
Down | Down Arrow
Select | Right Shift
Start | Enter
Reset | R

Press 'l' to enable/disable basic CPU logging. Disabled by default.


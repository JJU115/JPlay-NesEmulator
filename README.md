# JPlay-NesEmulator
An emulator of the Nintendo Entertainment System written in C++ written as a hobby project.


<h2>Current State</h2>
<p>
The system is working and can play most games without trouble. The only outstanding issues are:
<ul>
  <li>The DMC channel of the APU isn't functional</li>
  <li>Games that perform mid frame state changes (i.e split the the screen) may appear a little wonky</li>
</ul>
Aside from that JPlay implements and can play games using the following iNES mappers:
<ul>
  <li>(0) NROM</li>
  <li>(1) MMC1</li>
  <li>(2) UxROM</li>
  <li>(3) CxROM</li>
  <li>(4) MMC3</li>
  <li>(7) AxROM</li>
  <li>(9) MMC2</li>
  <li>(66) GxROM</li>
</ul>
This amounts to ~1412 different games.
</p>

<h2>Usage</h2>

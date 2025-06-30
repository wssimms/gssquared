
# Current Release: 0.2

## Release 0.3

[x] refactor all the other slot cards (like mb) to use the slot_store instead of device_store.  
[x] vector the RGB stuff as discussed in DisplayNG correctly.  
[x] make OSD fully match DisplayNG.  
[x] Fix the joystick.  
[x] video setup code should be put into its own area.  
[x] Implement general filter on speaker.cpp. (Ended up doing big rewrite to improve audio)  

## Release 0.3.5:

[x] refactor the hinky code we have in bus for handling mockingboard, I/O space memory switching, etc.  
[x] Tracing  
[x] Debugger  
[x] Bring in a decent readable font for the OSD elements  
[x] GS2 starts in powered-off mode. have snazzy tiles showing each platform that can be booted.  
[x] Can select a 'platform' each of which has a default configuration.  
[x] Can power on and power off vm  

## Release 0.4

[ ] Apple IIe support  
[ ] 80-column / double lo-res / double hi-res  
[ ] put common controls in hover-over on main page - reset, restart, power-off, open debugger
[ ] Refactor CPU to be more cycle-accurate including false/phantom reads/writes  
[ ] Implement new optimized audio code  
[ ] Implement cycle-accurate video display to support apps that switch video mode by counting cycles  
[ ] implement floating-bus read based on video data  

[ ] provide a mode for Atari Joyport - use the dpad. https://lukazi.blogspot.com/2009/04/game-controller-atari-joysticks.html. Can also use gamepad.    

[ ] Can edit slots / hw config when powered off.  
[ ] when we go to power off (from inside OSD), check to see if disks need writing, and throw up appropriate dialogs.  
[ ] put "modified" indicator of some kind on the disk icons.  


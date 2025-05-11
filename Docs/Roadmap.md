
# Current Release: 0.2

## Release 0.3

[x] refactor all the other slot cards (like mb) to use the slot_store instead of device_store.  
[x] vector the RGB stuff as discussed in DisplayNG correctly.  
[x] make OSD fully match DisplayNG.  
[ ] refactor the hinky code we have in bus for handling mockingboard, I/O space memory switching, etc.  
[x] Fix the joystick.  
[ ] implement floating-bus read based on calculated video scan position.  
[x] video setup code should be put into its own area.  

## Release 0.35:

[ ] Tracing  
[ ] Debugger  
[ ] Bring in a decent readable font for the OSD elements  

## Release 0.4

[ ] provide a mode for Atari Joyport - use the dpad. https://lukazi.blogspot.com/2009/04/game-controller-atari-joysticks.html  

[ ] GS2 starts in powered-off mode.  
[ ] can power on and power off.  
[ ] can edit slots / hw config when powered off.  
[ ] "powered off" means everything is shut down and deallocated, only running event loop and OSD.  
[ ] when we go to power off (from inside OSD), check to see if disks need writing, and throw up appropriate dialogs.  
[ ] put "modified" indicator of some kind on the disk icons.  
[ ] Implement general filter on speaker.cpp.  


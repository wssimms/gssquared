
# Current Release: 0.2

## Release 0.3

[x] refactor all the other slot cards (like mb) to use the slot_store instead of device_store.  
[x] vector the RGB stuff as discussed in DisplayNG correctly.  
[x] make OSD fully match DisplayNG.  
[x] Fix the joystick.  
[x] video setup code should be put into its own area.  
[x] Implement general filter on speaker.cpp. (Ended up doing big rewrite to improve audio)  

## Release 0.3.5:

[ ] Implement new optimized audio code  
[ ] implement floating-bus read based on calculated video scan position.  
[x] refactor the hinky code we have in bus for handling mockingboard, I/O space memory switching, etc.  
[ ] Tracing  
[ ] Debugger  
[x] Bring in a decent readable font for the OSD elements  

## Release 0.4

[ ] provide a mode for Atari Joyport - use the dpad. https://lukazi.blogspot.com/2009/04/game-controller-atari-joysticks.html. Can also use gamepad.    

[ ] GS2 starts in powered-off mode. have snazzy tiles showing each platform that can be booted.  
[ ] can power on and power off.  
[ ] can edit slots / hw config when powered off.  
[ ] "powered off" means everything is shut down and deallocated, only running event loop and OSD.  
[ ] when we go to power off (from inside OSD), check to see if disks need writing, and throw up appropriate dialogs.  
[ ] put "modified" indicator of some kind on the disk icons.  


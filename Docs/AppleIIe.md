# Apple IIe Functionality Checklist

Here is the Apple IIe checklist:

[x] 128K memory management  
[x] 80 column text  
[x] dlgr  
[x] dhgr  
[x] VBL  
[ ] Alternate character set (I implemented the switch but don't act on it yet)  
[x] display must read video memory from absolute memory, not from page table
[x] have "default to rom" concept in mmu. (mostly resolved by moving switches into mmu)  
[x] modifier keys mapped to open-apple and closed-apple  
[x] up and down arrow keys  
[x] AKD flag  
[x] read last key press from anywhere in C000-C01F  
[ ] implement C040 strobe - related to game I/O  it's a strobe output unused. (?)  
[x] C071-7F same as C070 (missing in apple ii+ impl also)  
[ ] on control-oa-reset don't clear locations like on ii+  
[ ] self-test fails mmu flag: e4:D

## Notes

### read last key press from anywhere in c000-c01f

we are missing: c011, c012, c015, c01e, c01f.

This is ugly. This should be done using the messaging bus concept, instead of creating coupling between modules.

### AKD

this is "any key down". The current implementation checking GetKeyboardState isn't great (I have to scan an array that could be quite big every time). Glider works ok with it. Sorta. "AKD" includes shift, so if I hit an arrow and let go, but then hit shift and hold that down, it moves because it's reusing the previous key down value. Yeah, the array is of 512 keys and we're iterating that every time. Icky.

So let's monitor key up and key down and track the difference, and if "keysdown>0" then AKD is true. We need to disregard just modifiers for these purposes.


### VBL

Glider is sometimes becoming invisible, then coming back. Seems clearly to be a vbl issue. I currently allocate 4,550 cycles for vbl. They could be de-synchronized compared to our actual screen updates. hmm. main loop is 17,008 cycles. that's 60hz. Apple II video is actually 59.9x Hz. well let's try vbl at 17,008. starts out invisible but so far has been good. I think we need to start the count of 4,550 cycles at the start of a frame. This is in display, so the update_apple2_display thing can set the vbl cycle start.

The issue is we're never going to divide by exactly how many cycles went by, using a fixed count because the actual number of cycles in the cpu execution loop is going to vary depending on the exact instructions (some instructions may run long).

ok, this is working better so far off the bat. The glider showed up first thing and now I'll sit here and watch and see what happens. Lots better!!

### Read last key press from anywhere in C000-C01F

most of these are in iie memory but not all of them; they need to call iiekbd to get the "last key value" and return that. This is all IIe stuff, so, we can have these modules be dependent.

### Modifier Keys / OA-CA

This is fine, per discussion in DevelopLog. we might need to check platform windows/linux and switch these to ALT.

### Self-Test fails: 80 store switch won't change

Getting an MMU E4:D code. That is "80store switch won't change". That's not right..

actually, code trace seems to indicate the issue is: c00b, then c00a, then read c017. this is -not- 80store. This is SLOTC3ROM.
Ahh, slotc3rom wasn't working correctly. The message was wrong! it's in the mmu but iiememory was overwriting it.
Now the self-test passes. I wonder if some recent things that weren't working will work now..


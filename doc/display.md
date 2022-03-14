## TECO-64 - Display Mode Commands

### W Commands

W commands are an optional feature of TECO that allows display mode
to be enabled or disabled, and also allows display mode parameters to
be read or set.

Display mode in TECO is handled by using the *ncurses* functions.
These do the work formerly performed directly by other TECOs. For
this reason, many of the W command used in those TECOs are unnecessary
and were not implemented in TECO-64.

| Command | Function |
| ------- | -------- |
| -1W | Enable display mode. |
| W   | Disable display mode. |

The :W commands are used to read or set (see m,n:W below) display
mode information.

| Command | Function |
| ------- | -------- |
| 0:W | The display terminal type. Not used in TECO-64. For compatibility with older TECO macros, this returns 8, to indicate VT102 in ANSI mode. |
| :W | Equivalent to 0:W. |
| 1:W | Return the horizontal size of the user’s display. |
| 2:W | Return the vertical size of the user’s display. |
| 3:W | Not implemented. |
| 4:W | Not implemented. |
| 5:W | Not implemented. |
| 6:W | Not implemented. |
| 7:W | Return or set the number of lines in the scrolling (command region) portion of the screen. If n is 0, then split screen scrolling is disabled. When split screen scrolling is enabled, n lines (as specified) are reserved at the bottom of the screen to scroll the terminal interaction. The remainder of the screen is used as a display window into the text buffer, and is automatically updated immediately before each command prompt. <br><br>The value of n must be greater than 1, and the display height minus n must be greater than 9. <br><br>Whenever the scrolling region’s size is modified (that is, whenever an m,7:W command is executed), TECO alters the display height (2:W) accordingly. For example, if 2:W is currently returning a value of 24, then after a 5,7:W command 2:W will return a value of 19. Executing a 0,7:W will restore 2:W to 24. |
| 8:W | If 0, enables scrolling regions. If non-zero, disables scrolling regions. |
| 9:W | Read-only terminal mask. For compatibility with older TECO macros, all bits are set, but none are used within TECO-64.<br><br>1 - Is ANSI CRT.<br>2 - Has EDIT mode features. <br>4 - Can do reverse scrolling. <br>8 - Has special graphics. <br>16 - Can do reverse video. <br>32 - Can change width. <br>64 - Has scrolling regions. <br>128 - Can erase to end-of-screen. |
| *m*,*n*:W | Sets the parameter represented by *n*:W to *m* and returns a value. If the new setting has been accepted, the returned value is *m*. Elsewise, the returned value is either the old value associated with *n*:W or whatever new setting was actually set. In all cases, the returned value reflects the new current setting. <br><br>Note that each *m*,*n*:W command returns a value, even if your only intent is to set something. Good programming practice suggests following any command which returns a value with \<*delim*\> or ^[ if you don’t intend that value to be passed to the following command. <br><br>Currently, the only parameters for which it makes any sense to set a value are 7:W and 8:W. |

### Color Commands

These commands set the foreground and background colors for the three
display regions: command region, edit region, and status line. Currently,
the following colors may be specified:

- black
- red
- yellow
- green
- blue
- cyan
- magenta
- white

These commands may optionally set the foreground and background
saturation levels, which may range from 0 (completely black) to
100 (maximum brightness).

The ^K command may be used to reset colors for all windows to their
defaults, to recover from a situation in which the a region has become
unreadable due to the foreground and background colors being set to
the same color.

| Command | Function |
| ------- | -------- |
| @F1/*color1*/*color2*/ | Set command region foreground color to *color1*, and the background color to *color2*. |
| *m*,*n*,@F1/*color1*/*color2*/ | Same as previous command, but sets the foreground and background saturation levels to *m* and *n*, respectively. If only *n* is specified, that sets the foreground saturation only. |
| @F2/*color1*/*color2*/ | Set edit region foreground color to *color1*, and the background color to *color2*. |
| *m*,*n*,@F2/*color1*/*color2*/ | Same as previous command, but sets the foreground and background saturation levels to *m* and *n*, respectively. If only *n* is specified, that sets the foreground saturation only. |
| @F3/*color1*/*color2*/ | Set status line foreground color to *color1*, and the background color to *color2*. |
| *m*,*n*,@F3/*color1*/*color2*/ | Same as previous command, but sets the foreground and background saturation levels to *m* and *n*, respectively. If only *n* is specified, that sets the foreground saturation only. |

| Example | Description |
| ------- | ----------- |
| @F1/green/black/ | Set foreground and background colors for the command window to green and black, respectively. |
| 70,80@F2/blue/white/ | Set the foreground color for the edit window to blue, at 70% saturation, and set the background color to white, at 80% saturation. |
| 60@F3/yellow/red/ | Set the foreground color for the status line to yellow, at 60% saturation, and set the background color to red (at a default saturation of 100%). |

///
///  @file    display.h
///  @brief   Header file for display mode functions.
///
///  @copyright 2019-2022 Franklin P. Johnston / Nowwith Treble Software
///
///  Permission is hereby granted, free of charge, to any person obtaining a
///  copy of this software and associated documentation files (the "Software"),
///  to deal in the Software without restriction, including without limitation
///  the rights to use, copy, modify, merge, publish, distribute, sublicense,
///  and/or sell copies of the Software, and to permit persons to whom the
///  Software is furnished to do so, subject to the following conditions:
///
///  The above copyright notice and this permission notice shall be included in
///  all copies or substantial portions of the Software.
///
///  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
///  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
///  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
///  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIA-
///  BILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
///  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
///  THE SOFTWARE.
///
////////////////////////////////////////////////////////////////////////////////

#if     !defined(_DISPLAY_H)

#define _DISPLAY_H

#include <stdbool.h>            //lint !e451

#include "teco.h"


#define SATMAX      1000            ///< Maximum color saturation

//  Values of foreground/background pairs for defined windows.

#define CMD         1               ///< Command window
#define EDIT        2               ///< Edit window
#define STATUS      3               ///< Status window
#define LINE        4               ///< Partition line
#define MAX_PAIRS   LINE            ///< Total no. of color pairs

///  @struct  tchar
///  @brief   Terminal characteristics flag.

union tchar
{
    struct
    {
        uint ansi_crt   : 1;        ///< Terminal is an ANSI CRT
        uint edit_mode  : 1;        ///< Terminal has EDIT mode features
        uint rev_scroll : 1;        ///< Terminal can do reverse scrolling
        uint spec_graph : 1;        ///< Terminal has special graphics
        uint rev_video  : 1;        ///< Terminal can do reverse video
        uint term_width : 1;        ///< Terminal can change its width
        uint scroll_reg : 1;        ///< Terminal has scrolling regions
        uint end_of_scr : 1;        ///< Terminal can erase to end of screen
    };

    uint flag;                      ///< Combined above flags
};

///  @struct  watch
///  @brief   Display mode variables

struct watch
{
    int type;                       ///< Type of scope
    int width;                      ///< Terminal width in columns
    int height;                     ///< Terminal height in rows
    int_t topdot;                   ///< Buffer position of upper left corner
    int_t botdot;                   ///< Buffer position of bottom right corner
    int nlines;                     ///< No. of scrolling lines
    int maxline;                    ///< Length of longest line in edit buffer
    bool seeall;                    ///< SEEALL mode
    bool noscroll;                  ///< Disable scrolling region
    union tchar tchar;              ///< Terminal characteristics
};


extern const bool esc_seq_def;

extern bool update_window;

extern struct watch w;


// Functions defined only if including display mode

extern void check_escape(bool escape);

extern int check_key(int c);

extern bool clear_eol(void);

extern void clear_dpy(void);

extern void color_dpy(void);

extern void end_dpy(void);

extern int exec_key(int c);

extern bool exec_soft(int key);

extern int get_nowait(void);

extern int get_tab(void);

extern int get_wait(void);

extern void init_charsize(void);

extern bool putc_cmd(int c);

extern void refresh_dpy(void);

extern void reset_colors(void);

extern void rubout_dpy(int c);

extern void set_parity(bool parity);

extern void set_tab(int n);

extern void start_dpy(void);

extern void unmark_dot(void);


#endif  // !defined(_DISPLAY_H)

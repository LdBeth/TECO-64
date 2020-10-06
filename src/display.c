///
///  @file    display.c
///  @brief   Display mode functions.
///
///  @copyright 2019-2020 Franklin P. Johnston / Nowwith Treble Software
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

#include <assert.h>

#if     defined(TECO_DISPLAY)

#include <ncurses.h>

#endif

#if     !defined(_STDIO_H)

#include <stdio.h>

#endif

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/ioctl.h>

#include "teco.h"

#if     defined(TECO_DISPLAY)

#include "ascii.h"

#endif

#include "display.h"
#include "editbuf.h"
#include "eflags.h"
#include "errors.h"
#include "term.h"


#if     defined(TECO_DISPLAY)

static const int tabsize = 8;       ///< Standard tab size

///
///  @def    err_if_true
///
///  @brief  Issue error if function returns specified value.
///

#define err_if_true(func, cond) if (func == cond) error_dpy()

#define MAX_POSITION    30          ///< Max. size of position string

///
///  @struct  region
///
///  @brief   Characteristics of a screen region
///

struct region
{
    int top;                        ///< Top of region
    int bot;                        ///< Bottom of region
};

///
///  @struct  display
///
///  @brief   Display format
///

struct display
{
    int row;                        ///< Text row
    int col;                        ///< Text column
    int vcol;                       ///< Virtual column
    int nrows;                      ///< No. of text rows
    struct region cmd;              ///< Command window
    struct region text;             ///< Text window
    struct region status;           ///< Status line
};

///
///  @var     d
///
///  @brief   Display format
///

static struct display d =
{
    .row    = 0,
    .col    = 0,
    .vcol   = 0,
    .nrows  = 0,
    .cmd    = { .top = 0, .bot = 0 },
    .text   = { .top = 0, .bot = 0 },
    .status = { .top = 0, .bot = 0 },
};


// Local functions

static void error_dpy(void);

static void exit_dpy(void);

static void mark_cursor(int row, int col);

static void move_down(void);

static void move_up(void);

static void repaint(int row, int col, int pos);

static void update_status(void);

#endif


///
///  @brief    Clear to end of line.
///
///  @returns  true if success, false if we couldn't.
///
////////////////////////////////////////////////////////////////////////////////

bool clear_eol(void)
{

#if     defined(TECO_DISPLAY)

    if (f.et.scope && f.e0.display)
    {
        (void)printw("\r");
        (void)clrtoeol();
        (void)refresh();

        return true;
    }

#endif

    return false;
}


///
///  @brief    Clear screen and redraw display.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void clear_dpy(void)
{

#if     defined(TECO_DISPLAY)

    (void)clear();

    ebuf_changed = true;

    set_scroll(w.height, w.nlines);

    refresh_dpy();

#endif

}


///
///  @brief    Reset display and issue error. When TECO starts, we either try
///            to initialize for display mode, or initialize terminal settings.
///            So if display initialization fails, we have to ensure that the
///            terminal is set up correctly.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

#if     defined(TECO_DISPLAY)

static void error_dpy(void)
{
    reset_dpy();
    init_term();

    throw(E_DPY);                       // Display mode initialization
}

#endif


///
///  @brief    Reset display mode prior to exiting from TECO.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

#if     defined(TECO_DISPLAY)

static void exit_dpy(void)
{
    reset_dpy();
}

#endif


///
///  @brief    Get next character.
///
///  @returns  Character read, or -1 if no character available.
///
////////////////////////////////////////////////////////////////////////////////

#if     defined(TECO_DISPLAY)

int getchar_dpy(bool wait)

#else

int getchar_dpy(bool unused1)

#endif

{
    int c = 0;                          // Ensure that high bits are clear

#if     defined(TECO_DISPLAY)

    if (f.e0.display)
    {
        if (!wait)
        {
            (void)nodelay(stdscr, (bool)TRUE);
        }

        c = getch();

        if (!wait)
        {
            (void)nodelay(stdscr, (bool)FALSE);
        }

        if (c == KEY_BACKSPACE)
        {
            return DEL;
        }

        return c;
    }

    // If display mode support isn't compiled in, or it's not currently
    // active, then read a character through alternate means. Note that
    // if display mode is active, getch() always returns immediately,
    // which is why we usually call read() to get a single character.

    if (!wait)
    {
        return getch();
    }

#endif

    if (read(fileno(stdin), &c, 1uL) == -1)
    {
        return -1;
    }
    else
    {
        return c;
    }
}


///
///  @brief    Get terminal height and width.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void getsize_dpy(void)
{
    struct winsize ts;

    if (ioctl(fileno(stdin), (ulong)TIOCGWINSZ, &ts) == -1)
    {
        throw(E_SYS, NULL);             // Unexpected system error
    }

    w.width  = ts.ws_col;
    w.height = ts.ws_row;

#if     defined(TECO_DISPLAY)

    (void)resizeterm(w.height, w.width);

    set_nrows();

#endif
}


///
///  @brief    Initialize for display mode.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void init_dpy(void)
{

#if     defined(TECO_DISPLAY)

    if (!f.e0.display)
    {
        f.e0.display = true;

        static bool exit_set = false;

        if (!exit_set)
        {
            exit_set = true;

            register_exit(exit_dpy);
        }

        // Note that initscr() will print an error message and exit if it
        // fails to initialize, so there is no error return to check for.

        (void)initscr();

        err_if_true(cbreak(),                       ERR);
        err_if_true(noecho(),                       ERR);
        err_if_true(nonl(),                         ERR);
        err_if_true(notimeout(stdscr, (bool)TRUE),  ERR);
        err_if_true(idlok(stdscr,     (bool)TRUE),  ERR);
        err_if_true(scrollok(stdscr,  (bool)TRUE),  ERR);
        err_if_true(keypad(stdscr,    (bool)FALSE), ERR);
        err_if_true(has_colors(),                   FALSE);
        err_if_true(start_color(),                  ERR);

        reset_colors();
        (void)set_escdelay(0);

        (void)attrset(COLOR_PAIR(CMD)); //lint !e835 !e845

        set_nrows();

        d.vcol = 0;
    }

#else

    throw(E_NOD);                       // Display mode not enabled

#endif

}


///
///  @brief    Mark or unmark cursor at current position.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

#if     defined(TECO_DISPLAY)

static void mark_cursor(int row, int col)
{
    // Save current position

    int saved_row, saved_col;
    uint c;

    getyx(stdscr, saved_row, saved_col);

    (void)attrset(COLOR_PAIR(TEXT));    //lint !e835

    // Go to old cursor position

    (void)move(d.text.top + d.row, d.col);

    c = inch() & A_CHARTEXT;            //lint !e835

    (void)delch();
    (void)insch(c);

    // Go to new cursor position

    d.row = row;
    d.col = col;

    (void)move(d.text.top + d.row, d.col);

    c = inch() | A_REVERSE;

    (void)delch();
    (void)insch(c);

    // Restore old position and color

    (void)attrset(COLOR_PAIR(CMD));     //lint !e835 !e845
    (void)move(d.text.top + saved_row, saved_col);
}

#endif


///
///  @brief    Move cursor down.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

#if     defined(TECO_DISPLAY)

static void move_down(void)
{
    int line = getlines_ebuf(-1);       // Get current line number
    int row = d.row;
    int col = d.col;

    if (line == getlines_ebuf(0))       // On last line?
    {
        return;
    }

    if (row == d.nrows - 1)
    {
        // TODO: scroll up

        return;
    }

    ++row;

    int next = getdelta_ebuf(1);        // Start of next line
    int len = getdelta_ebuf(2) - next;  // Length of next line
    int dot = t.dot + next;

    if (d.vcol < col)
    {
        d.vcol = col;
    }

    col = d.vcol;

    if (len < col)
    {
        dot += len - 1;
        col = len - 1;
    }
    else
    {
        dot += col;
    }

    if (dot > t.Z)                      // Make sure we stay within buffer
    {
        dot = t.Z;
    }

    mark_cursor(row, col);

    setpos_ebuf(dot);

    update_status();

    (void)refresh();
}

#endif


///
///  @brief    Move cursor up.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

#if     defined(TECO_DISPLAY)

static void move_up(void)
{
    int line = getlines_ebuf(-1);       // Get current line number
    int row = d.row;
    int col = d.col;

    if (line == 0)                      // On first line?
    {
        return;                         // Yes, nothing to do
    }

    if (row == 0)
    {
        // TODO: scroll lines down
        return;
    }

    --row;

    int prev = -getdelta_ebuf(-1);      // Distance to start of previous
    int len = prev - col;               // Length of previous line
    int dot = t.dot - prev;

    if (d.vcol < col)
    {
        d.vcol = col;
    }

    col = d.vcol;

    if (len < col)
    {
        dot += len - 1;
        col = len - 1;
    }
    else
    {
        dot += col;
    }

    mark_cursor(row, col);

    setpos_ebuf(dot);

    update_status();

    (void)refresh();
}

#endif


///
///  @brief    Output character to display. We do not output CR because ncurses
///            does the following when processing LF:
///
///            1. Clear to end of line.
///            2. Go to start of next line.
///
///            So, not only is CR not necessary, but if it preceded LF, this
///            would result in the current line getting blanked.
///
///  @returns  true if character output, false if display not active.
///
////////////////////////////////////////////////////////////////////////////////

#if     defined(TECO_DISPLAY)

bool putc_dpy(int c)

#else

bool putc_dpy(int unused1)

#endif

{

#if     defined(TECO_DISPLAY)

    if (f.e0.display)
    {
        if (c != CR)
        {
            (void)addch((uint)c);
            (void)refresh();
        }

        return true;
    }

#endif

    return false;
}


///
///  @brief    Output string to display.
///
///  @returns  true if character output, false if display not active.
///
////////////////////////////////////////////////////////////////////////////////

#if     defined(TECO_DISPLAY)

bool puts_dpy(const char *buf)

#else

bool puts_dpy(const char *unused1)

#endif

{

#if     defined(TECO_DISPLAY)

    if (f.e0.display)
    {
        (void)addstr(buf);
        refresh_dpy();

        return true;
    }

#endif

    return false;

}


///
///  @brief    Read display key.
///
///  @returns  Character to process, or EOF if already processed.
///
////////////////////////////////////////////////////////////////////////////////

int readkey_dpy(int key)
{

#if     defined(TECO_DISPLAY)

    if (key < KEY_MIN || key > KEY_MAX)
    {
        return key;
    }

    if (key == KEY_BACKSPACE)
    {
        return BS;
    }
    else if (key == KEY_UP)
    {
        move_up();
    }
    else if (key == KEY_DOWN)
    {
        move_down();
    }
    else if (key == KEY_LEFT || key == KEY_RIGHT)
    {
        int dot = (int)t.dot + (key == KEY_LEFT ? -1 : 1);

        if (dot >= t.B && dot <= t.Z)
        {
            setpos_ebuf(dot);
            refresh_dpy();
        }
    }

    return EOF;

#else

    return key;

#endif

}


///
///  @brief    Repaint screen.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

#if     defined(TECO_DISPLAY)

static void repaint(int row, int col, int pos)
{
    if (ebuf_changed)
    {
        ebuf_changed = false;

        d.vcol = 0;

        int saved_row, saved_col;

        getyx(stdscr, saved_row, saved_col); // Save position in command window

        (void)move(d.text.top, 0);      // Switch to text window
        (void)attrset(COLOR_PAIR(TEXT)); //lint !e835

        int c;
        int nrows = d.nrows;

        // Erase the current text window

        while (nrows-- > 0)
        {
            (void)addch('\n');
        }

        nrows = 0;

        (void)move(d.text.top, 0);      // Back to the top

        while ((c = getchar_ebuf(pos++)) != -1)
        {
            if (c == LF)
            {
                (void)move(d.text.top + nrows + 1, 0);
            }
            else if (c != CR)
            {
                (void)addch((uint)c);
            }

            if (isdelim(c) && ++nrows == d.nrows)
            {
                break;
            }
        }

        // Calculate visible column on screen, allowing for TABs.

        col = 0;
        pos = getdelta_ebuf(0);

        while (pos < 0)
        {
            if (getchar_ebuf(pos) == TAB)
            {
                col += tabsize - (col % tabsize);
            }
            else
            {
                ++col;
            }

            ++pos;
        }
        
        // Highlight our current position in text window

        (void)move(d.text.top + row, col);

        d.row = row;
        d.col = col;

        c = (int)inch();

        (void)delch();
        (void)insch((uint)c | A_REVERSE);

        // Restore position in command window

        (void)move(saved_row, saved_col);
        (void)attrset(COLOR_PAIR(CMD)); //lint !e835 !e845
    }

    update_status();

    (void)refresh();
}

#endif


///
///  @brief    Refresh screen.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void refresh_dpy(void)
{

#if     defined(TECO_DISPLAY)

    if (f.e0.display && w.nlines != 0)
    {
        int line = getlines_ebuf(-1);   // Line number within buffer
        int row  = line % d.nrows;      // Relative row within screen
        int col  = -getdelta_ebuf(0);   // Offset within row
        int pos  = getdelta_ebuf(-row); // First character to output

        repaint(row, col, pos);
    }

#endif

}


///
///  @brief    Reset window colors to defaults.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void reset_colors(void)
{

#if     defined(TECO_DISPLAY)

    if (can_change_color())             // Make colors as bright as possible
    {
        (void)init_color(COLOR_BLACK,        0,      0,      0);
        (void)init_color(COLOR_RED,     SATMAX,      0,      0);
        (void)init_color(COLOR_GREEN,        0, SATMAX,      0);
        (void)init_color(COLOR_YELLOW,  SATMAX, SATMAX,      0);
        (void)init_color(COLOR_BLUE,         0,      0, SATMAX);
        (void)init_color(COLOR_MAGENTA, SATMAX,      0, SATMAX);
        (void)init_color(COLOR_CYAN,         0, SATMAX, SATMAX);
        (void)init_color(COLOR_WHITE,   SATMAX, SATMAX, SATMAX);
    }

    (void)assume_default_colors(COLOR_BLACK, COLOR_WHITE);

    (void)init_pair(CMD,    COLOR_BLACK, COLOR_WHITE);
    (void)init_pair(TEXT,   COLOR_BLACK, COLOR_WHITE);
    (void)init_pair(STATUS, COLOR_WHITE, COLOR_BLACK);

#endif

}


///
///  @brief    Terminate display mode.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void reset_dpy(void)
{

#if     defined(TECO_DISPLAY)

    if (f.e0.display)
    {
        f.e0.display = false;

        (void)endwin();
    }

#endif

}


///
///  @brief    Set maximum no. of rows.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void set_nrows(void)
{

#if     defined(TECO_DISPLAY)

    d.nrows = w.height - w.nlines;

    if (f.e4.line)
    {
        --d.nrows;
    }

    assert(d.nrows > 0);                // Verify that we have at least 1 row

#endif

}


///
///  @brief    Set scrolling region.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

#if     defined(TECO_DISPLAY)

void set_scroll(int height, int nlines)

#else

void set_scroll(int unused1, int unused2)

#endif

{

#if     defined(TECO_DISPLAY)

    if (f.e0.display && w.nlines != 0)
    {
        if (f.e4.invert)
        {
            d.cmd.top  = 0;
            d.cmd.bot  = nlines - 1;
            d.text.top = nlines;
        }
        else
        {
            d.cmd.top  = height - nlines;
            d.cmd.bot  = height - 1;
            d.text.top = 0;
        }

        (void)setscrreg(d.cmd.top, d.cmd.bot);

        d.status.top = d.status.bot = -1;

        if (f.e4.line)
        {
            if (f.e4.invert)
            {
                d.status.top = d.status.bot = d.cmd.bot + 1;
                ++d.text.top;
            }
            else
            {
                d.status.top = d.status.bot = d.cmd.top - 1;
            }

            update_status();
        }

        (void)move(d.cmd.top, 0);

        for (int i = d.cmd.top; i <= d.cmd.bot; ++i)
        {
            (void)addch('\n');
        }

        (void)attrset(COLOR_PAIR(CMD)); //lint !e835 !e845

        (void)move(d.cmd.top, 0);

        (void)refresh();

        set_nrows();
    }

#endif

}


///
///  @brief    Update status line.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

#if     defined(TECO_DISPLAY)

static void update_status(void)
{
    // Draw line between text window and command window

    int saved_row, saved_col;

    getyx(stdscr, saved_row, saved_col);

    (void)move(d.status.top, 0);
    (void)attrset(COLOR_PAIR(STATUS));  //lint !e835

    if (f.e4.status)
    {
        char status[w.width];

        memset(status, SPACE, (size_t)(uint)w.width);

        // Add some file status to the left side of the status line

        int row   = getlines_ebuf(-1);
        int nrows = getlines_ebuf(0);
        int col   = -getdelta_ebuf(0);
        char position[MAX_POSITION + 1];

        if (row < nrows)
        {
            snprintf(position, sizeof(position), "row=%d  col=%d",
                     row + 1, col);
        }
        else
        {
            strcpy(position, "<EOF>");
        }

        int nbytes = snprintf(status, sizeof(status),
                              ".=%d  Z=%d  %s  nrows=%d  mem=%d",
                             t.dot, t.Z, position, nrows, t.size);

        status[nbytes] = SPACE;         // Replace NUL character with space

        // Now add in date and time on the right side of status line

        time_t now = time(NULL);
        struct tm *tm = localtime(&now);
        char tbuf[w.width];

        nbytes = (int)strftime(tbuf, sizeof(tbuf),
                               "%a, %e %b %Y, %H:%M %Z", tm);
        assert(nbytes != 0);            // Verify that we got some data

        memcpy(status + w.width - nbytes, tbuf, (size_t)(uint)nbytes);

        for (int i = 0; i < w.width; ++i)
        {
            int c = status[i];

            (void)addch((uint)c);
        }
    }
    else
    {
        for (int i = 0; i < w.width; ++i)
        {
            (void)addch(ACS_HLINE);
        }
    }

    (void)move(saved_row, saved_col);
    (void)attrset(COLOR_PAIR(CMD));     //lint !e835 !e845
}

#endif

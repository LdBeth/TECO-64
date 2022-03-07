///
///  @file    display.c
///  @brief   Display mode functions.
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

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <ncurses.h>

#if     !defined(_STDIO_H)

#include <stdio.h>

#endif

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "teco.h"
#include "ascii.h"
#include "errcodes.h"
#include "display.h"
#include "editbuf.h"
#include "eflags.h"
#include "estack.h"
#include "exec.h"
#include "page.h"
#include "term.h"


#if     INT_T == 64

#define DEC_FMT "%-*ld"             ///< Left-justified decimal format

#else

#define DEC_FMT "%-*d"              ///< Left-justified decimal format

#endif

const bool esc_seq_def = true;      ///< Escape sequences enabled by default

static bool dot_changed = false;    ///< true if dot changed

static bool ebuf_changed = false;   ///< true if edit buffer modified

static int rowbias = 0;             ///< Row adjustment

static uint n_home = 0;             ///< No. of consecutive Home keys

static uint n_end = 0;              ///< No. of consecutive End keys


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
    int row;                        ///< Edit row
    int col;                        ///< Edit column
    int vcol;                       ///< Virtual column
    int nrows;                      ///< No. of edit rows
    struct region cmd;              ///< Command region
    struct region edit;             ///< Edit region
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
    .edit   = { .top = 0, .bot = 0 },
    .status = { .top = 0, .bot = 0 },
};


/// @def    check_error(truth)
/// @brief  Wrapper to force Boolean value for check_error() parameter.

#if     !defined(DOXYGEN)

#define check_error(truth) (check_error)((bool)(truth))

#endif

// Local functions

static void (check_error)(bool truth);

static void exec_commands(const char *string);

static int geteditsize(char *buf, ulong size, uint_t bytes);

static int getwidth(ulong n);

static void init_dpy(void);

static void mark_cursor(int row, int col);

static void move_down(void);

static void move_left(void);

static void move_right(void);

static void move_up(void);

static int print_ebuf(char *buf, int width, int nbytes, int c);

static void reset_dpy(void);

static void resize_key(void);

static void update_status(void);


///
///  @brief    Check for special display input characters.
///
///  @returns  Next input character to process.
///
////////////////////////////////////////////////////////////////////////////////

int check_dpy_chr(int c, bool wait)
{
    if (c == KEY_BACKSPACE)
    {
        return DEL;
    }
    else if (c == KEY_RESIZE)
    {
        resize_key();

        return getc_term(wait);         // Recurse to get next character
    }
    else
    {
        return c;
    }
}


///
///  @brief    Issue an error if caller's function call failed.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

#if     defined(DOXYGEN)
static void check_error(bool truth)
#else
static void (check_error)(bool truth)
#endif
{
    if (truth)
    {
        reset_dpy();
        init_term();

        throw(E_DPY);                       // Display mode initialization
    }
}


///
///  @brief    Check to see if escape sequences were enabled or disabled.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void check_escape(bool escape)
{
    (void)keypad(stdscr, escape ? (bool)TRUE : (bool)FALSE);
}


///
///  @brief    Clear screen and redraw display.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void clear_dpy(void)
{
    term_pos = 0;

    (void)clear();

    ebuf_changed = true;

    set_scroll(w.height, w.nlines);

    refresh_dpy();
}


///
///  @brief    Clear to end of line.
///
///  @returns  true if success, false if we couldn't.
///
////////////////////////////////////////////////////////////////////////////////

bool clear_eol(void)
{
    if (f.et.scope && f.e0.display)
    {
        (void)printw("\r");
        (void)clrtoeol();
        (void)refresh();

        return true;
    }

    return false;
}


///
///  @brief    Get length of echoed character to be rubbed out.
///
///  @returns  Length in bytes, or EOF if length not available.
///
////////////////////////////////////////////////////////////////////////////////

int echo_len(int c)
{
    if (f.e0.display)
    {
        const char *s = unctrl((uint)c);

        if (s != NULL)
        {
            return (int)strlen(s);
        }
    }

    return EOF;
}


///
///  @brief    Check for ending display mode.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void end_dpy(void)
{
    if (f.e0.display)
    {
        reset_dpy();
        init_term();
    }
}


///
///  @brief    Execute command string.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

static void exec_commands(const char *commands)
{
    assert(commands != NULL);

    uint_t nbytes = (uint_t)strlen(commands);
    char text[nbytes + 1];

    strcpy(text, commands);

    tbuffer buf;

    buf.data = text;
    buf.size = nbytes;
    buf.len  = buf.size;
    buf.pos  = 0;

    bool saved_exec = f.e0.exec;

    // The reason for the next line is that we are called from readkey_dpy(),
    // which in turn is called when we are processing character input. So the
    // execution flag isn't on at this point, but we need to temporarily force
    // it in order to process an immediate-mode command string initiated by a
    // special key such as Page Up or Page Down.

    f.e0.exec = true;                   // Force execution

    exec_macro(&buf, NULL);

    f.e0.exec = saved_exec;             // Restore previous state

    refresh_dpy();
}


///
///  @brief    Reset display mode prior to exiting from TECO.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void exit_dpy(void)
{
    reset_dpy();
}


///
///  @brief    Get size of edit buffer.
///
///  @returns  No. of bytes written to buffer.
///
////////////////////////////////////////////////////////////////////////////////

static int geteditsize(char *buf, ulong size, uint_t bytes)
{
    assert(buf != NULL);

    if (bytes >= GB)
    {
        return snprintf(buf, size, "%uG", (uint)(bytes / GB));
    }
    else if (bytes >= MB)
    {
        return snprintf(buf, size, "%uM", (uint)(bytes / MB));
    }
    else if (bytes >= KB)
    {
        return snprintf(buf, size, "%uK", (uint)(bytes / KB));
    }
    else
    {
        return snprintf(buf, size, "%u", (uint)bytes);
    }
}


///
///  @brief    Get width of unsigned number (basically, log10() without using
///            a floating-point library function).
///
///  @returns  No. of bytes written to buffer.
///
////////////////////////////////////////////////////////////////////////////////

static int getwidth(ulong bytes)
{
    char buf[12];

    return snprintf(buf, sizeof(buf), "%lu", bytes);
}


///
///  @brief    Read next character without wait (non-blocking I/O).
///
///  @returns  Character read.
///
////////////////////////////////////////////////////////////////////////////////

int get_nowait(void)
{
    if (f.e0.display)
    {
        (void)nodelay(stdscr, (bool)TRUE);

        int c = getch();

        (void)nodelay(stdscr, (bool)FALSE);

        return c;
    }
    else
    {
        return get_wait();
    }
}


///
///  @brief    Read next character (if in display mode).
///
///  @returns  Character read, or EOF if none available.
///
////////////////////////////////////////////////////////////////////////////////

int get_wait(void)
{
    int c = getch();

    if (c != ERR)
    {
        return c;
    }

    return EOF;
}


///
///  @brief    Initialize for display mode.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

static void init_dpy(void)
{
    if (!f.e0.display)
    {
        bool esc_seq = f.ed.escape ? (bool)TRUE : (bool)FALSE;

        f.e0.display = true;

        // Note that initscr() will print an error message and exit if it
        // fails to initialize, so there is no error return to check for.

        (void)initscr();

        check_error( cbreak()                       == ERR   );
        check_error( noecho()                       == ERR   );
        check_error( nonl()                         == ERR   );
        check_error( notimeout(stdscr, (bool)TRUE)  == ERR   );
        check_error( idlok(stdscr,     (bool)TRUE)  == ERR   );
        check_error( scrollok(stdscr,  (bool)TRUE)  == ERR   );
        check_error( has_colors()                   == FALSE );
        check_error( start_color()                  == ERR   );
        check_error( keypad(stdscr, esc_seq)        == ERR   );

        reset_colors();
        (void)set_escdelay(0);

        (void)attrset(COLOR_PAIR(CMD)); //lint !e835 !e845

        set_nrows();
    }
}


///
///  @brief    Mark or unmark cursor at current position.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

static void mark_cursor(int row, int col)
{
    // Save current position

    int saved_row, saved_col;
    uint c;

    getyx(stdscr, saved_row, saved_col);

    (void)attrset(COLOR_PAIR(EDIT));    //lint !e835

    // Go to old cursor position

    (void)move(d.edit.top + d.row, d.col);

    c = inch() & A_CHARTEXT;            //lint !e835

    (void)delch();
    (void)insch(c);

    // Go to new cursor position

    d.row = row;
    d.col = col;

    (void)move(d.edit.top + d.row, d.col);

    c = inch() | A_REVERSE;

    (void)delch();
    (void)insch(c);

    // Restore old position and color

    (void)attrset(COLOR_PAIR(CMD));     //lint !e835 !e845
    (void)move(d.edit.top + saved_row, saved_col);
}


///
///  @brief    Mark dot as having changed.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void mark_dot(void)
{
    dot_changed = true;
}


///
///  @brief    Mark edit buffer as having changed.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void mark_ebuf(void)
{
    ebuf_changed = true;
}


///
///  @brief    Move cursor down.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

static void move_down(void)
{
    int_t line = getlines_ebuf(-1);     // Get current line number
    int row = d.row;
    int col = d.col;

    if (line == getlines_ebuf(0))       // On last line?
    {
        return;
    }

    if (row == d.nrows - 1)
    {
        ++rowbias;
    }

    ++row;

    int next = (int)getdelta_ebuf((int_t)1); // Start of next line
    int len = (int)getdelta_ebuf((int_t)2) - next; // Length of next line
    int_t dot = t.dot + next;

    if (col < d.vcol)
    {
        col = d.vcol;                   // Use virtual column if we can
    }

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

    dot_changed = false;                // Force this off for down arrow

    update_status();
    refresh_dpy();

    if (d.vcol < d.col)
    {
        d.vcol = d.col;                 // Update virtual column if needed
    }
}


///
///  @brief    Move cursor left.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

static void move_left(void)
{
    int_t dot = t.dot - 1;

    if (dot >= t.B)
    {
        int line = (int)getlines_ebuf(-1);
        int row = (line - rowbias) % d.nrows;

        setpos_ebuf(dot);

        if (row == 0 && line != getlines_ebuf(-1))
        {
            --rowbias;
        }

        refresh_dpy();

        d.vcol = d.col;                 // Update virtual column
    }
}


///
///  @brief    Move cursor right.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

static void move_right(void)
{
    int_t dot = t.dot + 1;

    if (dot <= t.Z)
    {
        int line = (int)getlines_ebuf(-1);
        int row = (line - rowbias) % d.nrows;

        setpos_ebuf(dot);

        if (row == d.nrows - 1 && line != getlines_ebuf(-1))
        {
            ++rowbias;
        }

        refresh_dpy();

        d.vcol = d.col;                 // Update virtual column
    }
}


///
///  @brief    Move cursor up.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

static void move_up(void)
{
    int line = (int)getlines_ebuf(-1);  // Get current line number
    int row = d.row;
    int col = d.col;

    if (line == 0)                      // On first line?
    {
        return;                         // Yes, nothing to do
    }

    if (row == 0)
    {
        --rowbias;
    }

    --row;

    int prev = (int)-getdelta_ebuf((int_t)-1); // Distance to start of previous
    int len = prev - col;               // Length of previous line
    int_t dot = t.dot - (int_t)prev;

    if (col < d.vcol)
    {
        col = d.vcol;                   // Use virtual column if we can
    }

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

    dot_changed = false;                // Force this off for up arrow

    update_status();
    refresh_dpy();

    if (d.vcol < d.col)
    {
        d.vcol = d.col;                 // Update virtual column if needed
    }
}


///
///  @brief    Output formatted description of edit buffer character.
///
///  @returns  No. of characters written to buffer.
///
////////////////////////////////////////////////////////////////////////////////

static int print_ebuf(char *buf, int width, int nbytes, int c)
{
    assert(buf != NULL);

    size_t size = (size_t)(uint)(width - nbytes);

    buf += nbytes;

    if (c == EOF)
    {
        return snprintf(buf, size, "----");
    }

    else if (isprint(c))
    {
        return snprintf(buf, size, "'%c' ", c);
    }
    else if (c > NUL && c < SPACE)
    {
        return snprintf(buf, size, "'^%c'", c + '@');
    }
    else
    {
        return snprintf(buf, size, "% 4d", c);
    }
}


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

bool putc_dpy(int c)
{
    if (f.e0.display)
    {
        if (c != CR)
        {
            (void)addch((uint)c);
        }

        return true;
    }

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
    if (!f.e0.display)
    {
        return key;
    }
    else if (exec_key(key))
    {
        ;
    }
    else if (key == KEY_HOME)
    {
        n_end = 0;

        if (++n_home == 1)              // Beginning of line
        {
            exec_commands("0L");
        }
        else if (n_home == 2)           // Beginning of window
        {
            exec_commands("F0J");
        }
        else                            // Beginning of file
        {
            exec_commands("0J");
        }

        return EOF;
    }
    else if (key == KEY_END)
    {
        n_home = 0;

        if (++n_end == 1)               // End of line
        {
            // We effectively execute "LR" to get to the end of a line that
            // ends with LF, and execute "L2R" for a line that ends with CR/LF.
            // The commands below, which include a test to see if the character
            // before the LF is a CR, take care of this regardless of the file
            // format.

            exec_commands("L (-2A-13)\"E 2R | R '");
        }
        else if (n_end == 2)            // End of window
        {
            exec_commands("(FZ-1)J");
        }
        else                            // End of file
        {
            exec_commands("ZJ");
        }

        return EOF;
    }
    else if (key == KEY_PPAGE)
    {
        exec_commands("-(2:W)L");
    }
    else if (key == KEY_NPAGE)
    {
        exec_commands("(2:W)L");
    }
    else if (key == KEY_UP)
    {
        move_up();
    }
    else if (key == KEY_DOWN)
    {
        move_down();
    }
    else if (key == KEY_LEFT)
    {
        move_left();
    }
    else if (key == KEY_RIGHT)
    {
        move_right();
    }
    else if (key == CR || key == LF || key == ESC ||
            (key == ACCENT && f.et.accent) || key == f.ee)
    {
        if (w.nlines == 0 || w.noscroll)
        {
            exec_commands(".-Z \"N L T '");
        }
        else
        {
            exec_commands("L");
        }
    }
    else if (key == BS || key == DEL)
    {
        if (w.nlines == 0 || w.noscroll)
        {
            exec_commands(".-B \"N -L T '");
        }
        else
        {
            exec_commands("-L");
        }
    }
    else
    {
        n_home = n_end = 0;

        return key;
    }

    n_home = n_end = 0;

    return EOF;
}


///
///  @brief    Refresh screen.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void refresh_dpy(void)
{
    if (!f.e0.display || w.nlines == 0 || w.noscroll)
    {
        return;
    }

    int line = (int)getlines_ebuf(-1);  // Line number within buffer

    if (line == 0)
    {
        rowbias = 0;
    }

    int row  = (line - rowbias) % d.nrows; // Relative row within screen
    int_t pos  = getdelta_ebuf((int_t)-row); // First character to output

    if (ebuf_changed)
    {
        ebuf_changed = false;

        if (dot_changed)
        {
            dot_changed = false;

            d.vcol = 0;
        }

        int saved_row, saved_col;

        getyx(stdscr, saved_row, saved_col); // Save position in command region

        (void)move(d.edit.top, 0);      // Switch to edit region

        getyx(stdscr, d.row, d.col);    // Get starting position

        (void)attrset(COLOR_PAIR(EDIT)); //lint !e835

        int c;
        int nrows = d.nrows;

        // Erase the current edit region

        while (nrows-- > 0)
        {
            (void)addch('\n');
        }

        nrows = 0;

        (void)move(d.edit.top, 0);      // Back to the top

        uint line_pos = 0;              // Line position
        bool filled = false;            // Is edit region full?

        w.topdot = w.botdot = (int)(t.dot + pos);

        while ((c = getchar_ebuf(pos)) != EOF)
        {
            if (pos++ <= 0)
            {
                getyx(stdscr, d.row, d.col);
            }

            if (c == CR)
            {
                ++w.botdot;

                continue;
            }

            if (isdelim(c))
            {
                ++w.botdot;

                if (++nrows == d.nrows)
                {
                    filled = true;

                    break;
                }

                (void)move(d.edit.top + nrows, 0);

                line_pos = 0;
            }
            else
            {
                line_pos += (uint)strlen(unctrl((uint)c));

                if (line_pos > (uint)w.width)
                {
                    if (f.et.truncate)
                    {
                        ++w.botdot;

                        continue;
                    }

                    if (++nrows == d.nrows)
                    {
                        filled = true;

                        break;
                    }

                    (void)move(d.edit.top + nrows, 0);

                    line_pos = 0;
                }

                ++w.botdot;

                (void)addch((uint)c);
            }
        }

        // If at end of edit buffer, adjust cursor

        if (pos == 0)
        {
            getyx(stdscr, d.row, d.col);
        }

        // If at end of buffer, and if room for it, add marker

        if (!filled && getchar_ebuf(pos) == EOF)
        {
            (void)addch(A_ALTCHARSET | 0x60);
        }

        // Highlight our current position in edit region

        (void)move(d.row, d.col);

        c = (int)inch();

        (void)delch();
        (void)insch((uint)c | A_REVERSE);

        // Restore position in command region

        (void)move(saved_row, saved_col);
        (void)attrset(COLOR_PAIR(CMD)); //lint !e835 !e845
    }

    update_status();

    (void)refresh();
}


///
///  @brief    Reset region colors to defaults.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void reset_colors(void)
{
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
    (void)init_pair(EDIT,   COLOR_BLACK, COLOR_WHITE);
    (void)init_pair(STATUS, COLOR_WHITE, COLOR_BLACK);
}


///
///  @brief    Terminate display mode.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

static void reset_dpy(void)
{
    if (f.e0.display)
    {
        f.e0.display = false;

        (void)endwin();
    }
}


///
///  @brief    Finish window resize when KEY_RESIZE key read.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

static void resize_key(void)
{
    if (f.e0.display)
    {
        set_nrows();
        clear_dpy();
        print_prompt();
        echo_tbuf((uint_t)0);
    }
}


///
///  @brief    Start window resize when resize signal received.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void resize_signal(void)
{
    if (f.e0.display)
    {
        (void)resizeterm(w.height, w.width);
        getmaxyx(stdscr, w.height, w.width);
    }
}


///
///  @brief    Set maximum no. of rows.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void set_nrows(void)
{
    d.nrows = w.height - w.nlines;

    if (f.e4.line)
    {
        --d.nrows;
    }

    assert(d.nrows > 0);                // Verify that we have at least 1 row
}


///
///  @brief    Set scrolling region.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void set_scroll(int height, int nlines)
{
    if (f.e0.display && w.nlines != 0 && !w.noscroll)
    {
        if (f.e4.invert)
        {
            d.cmd.top  = 0;
            d.cmd.bot  = nlines - 1;
            d.edit.top = nlines;
        }
        else
        {
            d.cmd.top  = height - nlines;
            d.cmd.bot  = height - 1;
            d.edit.top = 0;
        }

        (void)setscrreg(d.cmd.top, d.cmd.bot);

        d.status.top = d.status.bot = -1;

        if (f.e4.line)
        {
            if (f.e4.invert)
            {
                d.status.top = d.status.bot = d.cmd.bot + 1;
                ++d.edit.top;
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
}


///
///  @brief    Check for starting display mode.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void start_dpy(void)
{
    if (!f.e0.display)
    {
        reset_term();                   // Reset if display mode support
        init_dpy();
        color_dpy();
        clear_dpy();
    }
}


///
///  @brief    Update status line.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

static void update_status(void)
{
    if (!f.e4.line)
    {
        return;
    }

    // Draw line between edit region and command region

    int saved_row, saved_col;

    getyx(stdscr, saved_row, saved_col);

    (void)move(d.status.top, 0);
    (void)attrset(COLOR_PAIR(STATUS));  //lint !e835

    if (f.e4.status)
    {
        char status[w.width];

        memset(status, SPACE, (size_t)(uint)w.width);

        // Add some file status to the left side of the status line

        int row     = (int)getlines_ebuf(-1);
        int nrows   = (int)getlines_ebuf(0);
        int col     = (int)-getdelta_ebuf((int_t)0);
        int width   = getwidth((ulong)(uint)t.Z);
        int nbytes  = snprintf(status, sizeof(status), ".=" DEC_FMT " (",
                               width, t.dot);
        size_t size = sizeof(status);   // Remaining bytes available in line

        nbytes += print_ebuf(status, w.width, nbytes, getchar_ebuf((int_t)-1));
        nbytes += snprintf(status + nbytes, size, ",");
        nbytes += print_ebuf(status, w.width, nbytes, getchar_ebuf((int_t)0));

        size = sizeof(status) - (size_t)(uint)nbytes;

        nbytes += snprintf(status + nbytes, size, ")  Z=" DEC_FMT " ",
                           width, t.Z);
        width = getwidth((ulong)(uint)nrows);

        if (t.dot >= t.Z)
        {
            nbytes += snprintf(status + nbytes, size,
                               "row=%-*d   <EOF>   ", width, row + 1);
        }
        else
        {
            nbytes += snprintf(status + nbytes, size,
                               "row=%-*d  col=%-3d  ", width, row + 1, col + 1);
        }

        size = sizeof(status) - (size_t)(uint)nbytes;

        nbytes += snprintf(status + nbytes, size, "nrows=%-*d  mem=", width,
                           nrows);

        size = sizeof(status) - (size_t)(uint)nbytes;

        nbytes += geteditsize(status + nbytes, size, getsize_ebuf());

        status[nbytes] = SPACE;         // Replace NUL character with space

        // Now add in page number on right side.

        char buf[w.width];

        nbytes = sprintf(buf, "Page %u", page_count());

        memcpy(status + w.width - nbytes, buf, (size_t)(uint)nbytes);

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

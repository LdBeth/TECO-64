///
///  @file    search.c
///  @brief   Search utility functions.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "teco.h"
#include "ascii.h"
#include "editbuf.h"
#include "eflags.h"
#include "errcodes.h"
#include "estack.h"
#include "exec.h"
#include "file.h"
#include "page.h"
#include "qreg.h"
#include "search.h"
#include "term.h"


///   @var    last_search
///   @brief  Last string searched for

tstring last_search = { .len = 0 };

// Local functions

static int isblankx(int c, struct search *s);

static int isctrlx(int c, int match);

static int isqreg(int c, struct search *s);

static int issymbol(int c);

static bool match_chr(int c, struct search *s);

static bool match_str(struct search *s);


///
///  @brief    Build a search string, allocating storage for it.
///
///  @returns  TECO string.
///
////////////////////////////////////////////////////////////////////////////////

void build_search(const char *src, uint_t len)
{
    tstring tmp = build_string(src, len);

    last_len = 0;                       // Assume search will fail

    free_mem(&last_search.data);

    last_search.data = alloc_mem(tmp.len + 1);
    last_search.len = tmp.len;

    strcpy(last_search.data, tmp.data);
}


///
///  @brief    Check for multiple blanks (spaces or tabs) at current position.
///
///            Note that we return 1/0 instead of true/false for compatibility
///            with the ANSI isxxx() functions.
///
///  @returns  1 if one or more blanks found, else 0.
///
////////////////////////////////////////////////////////////////////////////////

static int isblankx(int c, struct search *s)
{
    assert(s != NULL);                  // Error if no search block

    if (!isblank(c))
    {
        return 0;
    }

    while (s->text_pos < s->text_end)
    {
        if ((c = read_edit(s->text_pos++)) == EOF)
        {
            break;
        }
        else if (!isblank(c))
        {
            --s->text_pos;

            break;
        }
    }

    return 1;
}


///
///  @brief    Check for case-insensitive match, depending on the setting of
///            the CTRL/X flag:
///
///             1: Case-insensitive match.
///
///             0: Old case-insensitive match. Not only matches alphabetic
///                characters, but additionally the following pairs.
///
///                @ (64) and ` (96)
///                [ (91) and { (123)
///                | (92) and \ (124)
///                ] (93) and } (125)
///                ^ (94) and ~ (126)
///
///            -1: Case-sensitive match.
///
///            Note that we return 1/0 instead of true/false for compatibility
///            with the ANSI isxxx() functions.
///
///  @returns  1 if a match found, else 0.
///
////////////////////////////////////////////////////////////////////////////////

static int isctrlx(int c, int match)
{
    if (f.ctrl_x != -1)
    {
        c     = toupper(c);
        match = toupper(match);

        if (!isalpha(c) && f.ctrl_x == 0)
        {
            if (strchr("`{|}~", c) != NULL)
            {
                c -= 'a' - 'A';
            }

            if (strchr("`{|}~", match) != NULL)
            {
                match -= 'a' - 'A';
            }
        }
    }

    return (c == match) ? 1 : 0;
}


///
///  @brief    Check for a match with one of the characters in a Q-register.
///
///            Note that we return 1/0 instead of true/false for compatibility
///            with the ANSI isxxx() functions.
///
///  @returns  1 if a match found, else 0.
///
////////////////////////////////////////////////////////////////////////////////

static int isqreg(int c, struct search *s)
{
    assert(s != NULL);                  // Error if no search block

    int qname;
    bool qlocal = false;

    if (s->match_len-- == 0)
    {
        throw(E_MQN);                   // Missing Q-register name
    }

    if ((qname = *s->match_buf++) == '.')
    {
        qlocal = true;

        if (s->match_len-- == 0)
        {
            throw(E_MQN);               // Missing Q-register name
        }

        qname = *s->match_buf++;
    }

    int qindex = get_qindex(qname, qlocal);

    if (qindex == -1)
    {
        throw(E_IQN, qname);            // Invalid Q-register name
    }

    struct qreg *qreg = get_qreg(qindex);

    for (uint_t i = 0; i < qreg->text.len; ++i)
    {
        if (c == qreg->text.data[i])
        {
            return 1;
        }
    }

    return 0;
}


///
///  @brief    Check for a match on a symbol constituent: alphanumeric, period,
///            dollar sign and underscore.
///
///            Note that we return 1/0 instead of true/false for compatibility
///            with the ANSI isxxx() library functions.
///
///  @returns  1 if a match found, else 0.
///
////////////////////////////////////////////////////////////////////////////////

static int issymbol(int c)
{
    if (isalnum(c) || c == '.' || c == '$' || c == '_')
    {
        return 1;
    }
    else
    {
        return 0;
    }
}


///
///  @brief    Check for a match on the current character in the edit buffer,
///            allowing for the use of match control constructs in the search
///            string. Note that we can be called recursively.
///
///  @returns  true if a match found, else false.
///
////////////////////////////////////////////////////////////////////////////////

static bool match_chr(int c, struct search *s)
{
    assert(s != NULL);                  // Error if no search block

    if (s->match_len-- == 0)
    {
        throw(E_ISS);                   // Invalid search string
    }

    int match = *s->match_buf++;

    if (match == CTRL_E)
    {
        if (s->match_len-- == 0)
        {
            throw(E_ISS);               // Invalid search string
        }

        match = toupper(*s->match_buf++);

        if ((match == 'A' && isalpha(c))     ||
            (match == 'B' && !isalnum(c))    ||
            (match == 'C' && issymbol(c))    ||
            (match == 'D' && isdigit(c))     ||
            (match == 'G' && isqreg(c, s))   ||
            (match == 'L' && isdelim(c))     ||
            (match == 'R' && isalnum(c))     ||
            (match == 'S' && isblankx(c, s)) ||
            (match == 'V' && islower(c))     ||
            (match == 'W' && isupper(c))     ||
            (match == 'X'))
        {
            return true;
        }
        else if (strchr("ABCDGLRSVWX", match) != NULL)
        {
            return false;               // Valid match chr., but no match
        }

        // <CTRL/E>nnn matches character whose decimal value is nnn.

        if (match >= '0' && match <= '9')
        {
            int n = match - '0';

            // Loop until we run out of decimal digits

            while (s->match_len > 0)
            {
                if (!isdigit(*s->match_buf))
                {
                    break;
                }

                --s->match_len;

                n *= 10;                // Shift digit over
                n += *s->match_buf++ - '0'; // Add in new digit
            }

            return (c == n) ? true : false;
        }

        throw(E_ICE);                   // Invalid ^E command in search argument
    }
    else if (match == CTRL_N)           // ^N^N doesn't make sense
    {
        throw(E_ISS);                   // Invalid search string
    }
    else if ((match == CTRL_S && !isalnum(c)) ||
             match == CTRL_X                  ||
             isctrlx(c, match)                ||
             c == match)
    {
        return true;
    }

    return false;
}


///
///  @brief    Check to see if text string matches search string.
///
///  @returns  true if match, else false (unless the first character is CTRL/N,
///            if which case we return false if it's a match, otherwise true).
///
////////////////////////////////////////////////////////////////////////////////

static bool match_str(struct search *s)
{
    assert(s != NULL);                  // Error if no search block

    if (*s->match_buf == CTRL_N && s->match_len > 0)
    {
        --s->match_len;
        ++s->match_buf;

        while (s->match_len > 0)
        {
            int c = read_edit(s->text_pos++);

            if (c == EOF)
            {
                return false;
            }
            else if (!match_chr(c, s))
            {
                return true;
            }
        }

        return false;
    }
    else
    {
        while (s->match_len > 0)
        {
            int c = read_edit(s->text_pos++);

            if (c == EOF || !match_chr(c, s))
            {
                return false;
            }
        }

        return true;
    }
}


///
///  @brief    Deallocate memory for last search.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void reset_search(void)
{
    free_mem(&last_search.data);
}


///
///  @brief    Search backward through edit buffer to find next instance of
///            string in search buffer.
///
///  @returns  true if string found (text_pos will contain the buffer position
///            relative to dot), and false if not found.
///
////////////////////////////////////////////////////////////////////////////////

bool search_backward(struct search *s)
{
    assert(s != NULL);                  // Error if no search block

    // Start search at current position and see if we can get a match. If not,
    // decrement position by one, and try again. If we reach the end of the
    // edit buffer without a match, then return failure, otherwise update our
    // position and return success.

    while (s->text_start >= s->text_end) // Search to beginning of buffer
    {
        s->text_pos  = s->text_start--; // Start at current position
        s->match_len = last_search.len; // No. of characters left to match
        s->match_buf = last_search.data; // Start of match characters

        if (match_str(s))
        {
            return true;
        }
    }

    return false;
}


///
///  @brief    Process failure of any search command. If the command was colon-
///            modified, then we can just return a value. If not, but we are
///            in a loop, then we exit using an F> command. Otherwise, throw
///            an exception.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void search_failure(struct cmd *cmd, uint keepdot)
{
    assert(cmd != NULL);

    if (!keepdot)
    {
        set_dot(t->B);
    }

    if (cmd->colon)
    {
        push_x(FAILURE, X_OPERAND);
    }
    else
    {
        if (check_loop())
        {
            if (!check_semi())
            {
                tprint("%%Search failure in loop\n");
            }

            exit_loop(cmd);
        }
        else
        {
            last_search.data[last_search.len] = NUL;

            throw(E_SRH, last_search.data);
        }
    }
}


///
///  @brief    Search forward through edit buffer to find next instance of
///            string in search buffer.
///
///  @returns  true if string found (text_pos will contain the buffer position
///            relative to dot), and false if not found.
///
////////////////////////////////////////////////////////////////////////////////

bool search_forward(struct search *s)
{
    assert(s != NULL);                  // Error if no search block

    // Start search at current position and see if we can get a match. If not,
    // increment position by one, and try again. If we reach the end of the
    // edit buffer without a match, then return failure, otherwise update our
    // position and return success.

    while (s->text_start < s->text_end) // Search to end of buffer
    {
        s->text_pos  = s->text_start++; // Start at current position
        s->match_len = last_search.len; // No. of characters left to match
        s->match_buf = last_search.data; // Start of match characters

        if (match_str(s))
        {
            // The following affects how much we move dot on multiple occurrence
            // searches. Normally we skip over the whole matched string when
            // proceeding to the nth search match. But if movedot is set, then
            // only increment by one character. This is only significant if the
            // first character of the string occurs again in the middle of it.

            if (!f.ed.movedot)
            {
                s->text_start = s->text_pos;
            }

            return true;
        }
        else if (s->type == SEARCH_C)   // Are we processing ::S?
        {
            return false;               // Yes, then we've failed
        }
    }

    return false;
}


///
///  @brief    Search forward through edit buffer to find next instance of
///            string in search buffer.
///
///  @returns  true if string found (text_pos will contain the buffer position
///            relative to dot, and false if not found.
///
////////////////////////////////////////////////////////////////////////////////

bool search_loop(struct search *s)
{
    assert(s != NULL);                  // Error if no search block

    struct ifile *ifile = &ifiles[istream];
    struct ofile *ofile = &ofiles[ostream];

    // Start search at current position and see if we can get a match. If not,
    // increment position by one, and try again. If we reach the end of the
    // edit buffer without a match, then return failure, otherwise update our
    // position and return success.

    while (s->count > 0)
    {
        if ((*s->search)(s))            // Successful search?
        {
            --s->count;                 // Yes, count down occurrence
        }
        else
        {
            switch (s->type)
            {
                case SEARCH_N:
                    if (ofile->fp == NULL)
                    {
                        throw(E_NFO);   // No file for output
                    }

                    if (s->search == search_backward)
                    {
                        if (!page_backward((int_t)-1, f.ctrl_e))
                        {
                            return false;
                        }

                        set_dot(t->Z);  // Go to end of buffer

                        s->text_start = -1;
                        s->text_end = -t->Z;
                    }
                    else if (!next_page((int_t)0, t->Z, f.ctrl_e, (bool)true))
                    {
                        return false;
                    }

                    break;

                case SEARCH_U:
                    if (!f.ed.yank && ofiles[ostream].fp != NULL)
                    {
                        throw(E_YCA);   // Y command aborted
                    }
                    //lint -fallthrough

                case SEARCH_E:          // For E_ commands
                    if (ifile->fp == NULL)
                    {
                        throw(E_NFI);   // No file for input
                    }

                    if (s->search == search_backward)
                    {
                        yank_backward(NULL);

                        if (t->Z == 0)
                        {
                            return false;
                        }

                        set_dot(t->Z);  // Go to end of buffer

                        s->text_start = -1;
                        s->text_end = -t->Z;
                    }
                    else
                    {
                        if (!next_yank())
                        {
                            return false;
                        }

                        set_dot(t->B);
                    }

                    break;

                default:
                case SEARCH_S:
                case SEARCH_C:
                    return false;
            }

            // Here with a new page, so reinitialize pointers

            if (s->search == search_backward)
            {
                s->text_start = -1;     // Start at previous character
                s->text_end   = -t->Z;
            }
            else
            {
                s->text_start = 0;      // Start at current character
                s->text_end   = t->Z;
            }
        }
    }

    set_dot(t->dot + s->text_pos);

    last_len = last_search.len;         // Save length of last search string

    return true;
}


///
///  @brief    Process success of any search command.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void search_success(struct cmd *cmd)
{
    assert(cmd != NULL);

    if (cmd->colon || (check_loop() && check_semi()))
    {
        push_x(SUCCESS, X_OPERAND);
    }
}

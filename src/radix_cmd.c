///
///  @file    radix_cmd.c
///  @brief   Execute radix commands.
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
#include <stdio.h>
#include <stdlib.h>

#include "teco.h"
#include "errors.h"
#include "estack.h"
#include "exec.h"


///
///  @brief    Execute ^D (CTRL/D) command: switch radix to decimal.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void exec_ctrl_D(struct cmd *unused1)
{
    radix = 10;                         // Set radix to decimal
}


///
///  @brief    Execute ^O (CTRL/O) command: switch radix to octal.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void exec_ctrl_O(struct cmd *unused1)
{
    radix = 8;                          // Set radix to octal
}


///
///  @brief    Execute ^R (CTRL/R) command: read current radix.
///
///  @returns  nothing.
///
////////////////////////////////////////////////////////////////////////////////

void exec_ctrl_R(struct cmd *cmd)
{
    assert(cmd != NULL);                // Error if no command block

    if (cmd->n_set)                     // n^R?
    {
        if (cmd->n_arg != 8 && cmd->n_arg != 10 && cmd->n_arg != 16)
        {
            throw(E_IRA);               // Illegal radix argument
        }

        radix = cmd->n_arg;             // Set the radix
    }
    else
    {
        push_expr(radix, EXPR_VALUE);   // No, just save what we have
    }
}

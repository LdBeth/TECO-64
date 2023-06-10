///
///  @file    ctrl_w_cmd.c
///  @brief   Execute CTRL/W command.
///
///  @copyright 2019-2023 Franklin P. Johnston / Nowwith Treble Software
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
#include "eflags.h"
#include "estack.h"
#include "exec.h"


///
///  @brief    Execute CTRL/W command: force conversion to upper case.
///
///             ^W -> enable upper-case conversion.
///            0^W -> disable upper-case conversion.
///            n^w -> enable upper-case conversion if n is non-zero.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void exec_ctrl_W(struct cmd *cmd)
{
    assert(cmd != NULL);

    scan_x(cmd);
    confirm(cmd, NO_M, NO_COLON, NO_DCOLON, NO_ATSIGN);

    if (cmd->n_set && cmd->n_arg == 0)
    {
        f.e0.upper = false;
    }
    else
    {
        f.e0.upper = true;
    }

    f.e0.lower = false;
}

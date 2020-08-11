///
///  @file    ew_cmd.c
///  @brief   Execute EW command.
///
///  @bug     No known bugs.
///
///  @copyright  2019-2020 Franklin P. Johnston / Nowwith Treble Software
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
#include "estack.h"
#include "exec.h"
#include "file.h"


///
///  @brief    Execute EW command (open file for output)
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void exec_EW(struct cmd *cmd)
{
    assert(cmd != NULL);

    const char *buf = cmd->text1.buf;
    uint len = cmd->text1.len;

    if (len == 0)                       // EW`?
    {
        ostream = OFILE_PRIMARY;        // Yes, switch to primary output stream
        set_last(ofiles[ostream].name);

        return;
    }

    assert(buf != NULL);

    struct ofile *ofile = open_output(buf, len, ostream, cmd->colon, 'W');

    ofile->backup = true;               // Create backup file on close

    // Note: open_output() only returns NULL for colon-modified command.

    if (ofile == NULL)
    {
        push_expr(0, EXPR_VALUE);
    }
    else if (cmd->colon)
    {
        push_expr(-1, EXPR_VALUE);
    }
}

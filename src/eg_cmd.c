///
///  @file    eg_cmd.c
///  @brief   Execute EG command.
///
///  @copyright 2019-2021 Franklin P. Johnston / Nowwith Treble Software
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
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "teco.h"
#include "ascii.h"
#include "editbuf.h"
#include "errcodes.h"
#include "estack.h"
#include "exec.h"
#include "file.h"


char eg_command[PATH_MAX] = { NUL };    ///< Command to execute on exit


///
///  @brief    Execute EG command: execute system command.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void exec_EG(struct cmd *cmd)
{
    assert(cmd != NULL);

    if (cmd->text1.len > sizeof(eg_command) - 1)
    {
        throw(E_CMD);                   // System command is too long
    }

    if (cmd->colon)
    {
        char syscmd[PATH_MAX];          // System command

        snprintf(syscmd, (ulong)PATH_MAX, "%.*s", (int)cmd->text1.len,
                 cmd->text1.data);

        int status = find_eg(syscmd);

        push_x((int_t)status, X_OPERAND);

        return;
    }

    tstring eg = build_string(cmd->text1.data, cmd->text1.len);

    snprintf(eg_command, sizeof(eg_command), "%s", eg.data);

    // The following ensures that we don't exit if we have nowhere to output
    // the data in the buffer to.

    struct ofile *ofile = &ofiles[ostream];

    if (ofile->fp == NULL && t.Z != 0)
    {
        throw(E_NFO);                   // No file for output
    }

    close_files();

    // EG`, not :EG`, so get ready to exit

    exit(EXIT_SUCCESS);
}

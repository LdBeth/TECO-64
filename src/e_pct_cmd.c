///
///  @file    e_pct_cmd.c
///  @brief   Execute E% command.
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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "teco.h"
#include "errors.h"
#include "exec.h"
#include "estack.h"
#include "file.h"
#include "qreg.h"


///
///  @brief    Execute E% command: write Q-register to file.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void exec_E_pct(struct cmd *cmd)
{
    assert(cmd != NULL);

    const char *name = cmd->text1.data;
    uint_t len       = cmd->text1.len;
    uint stream      = OFILE_QREGISTER;

    if (len == 0)                       // Any file name?
    {
        return;                         // No, so it's a no-op
    }

    assert(name != NULL);               // Error if no name

    if ((name = init_filename(name, len, cmd->colon)) != NULL)
    {
        struct ofile *ofile;

        if ((ofile = open_output(name, stream, cmd->colon, '%')) != NULL)
        {
            struct qreg *qreg = get_qreg(cmd->qindex);

            assert(qreg != NULL);       // Error if no Q-register

            if (qreg->text.len > 0)
            {
                size_t size = (size_t)(uint)qreg->text.len;

                if (fwrite(qreg->text.data, 1uL, size, ofile->fp) != size)
                {
                    throw(E_ERR, ofile->name); // General error
                }
            }

            close_output(stream);

            if (cmd->colon)
            {
                store_val(SUCCESS);
            }

            return;
        }
    }

    // Only here if error occurred when colon modifier specified.

    store_val(FAILURE);
}

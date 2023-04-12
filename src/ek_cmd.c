///
///  @file    ek_cmd.c
///  @brief   Execute EK command.
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
#include <string.h>

#include "teco.h"
#include "errcodes.h"
#include "exec.h"
#include "file.h"
#include "page.h"


///
///  @brief    Execute EK command: kill current output file.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void exec_EK(struct cmd *cmd)
{
    scan_x(cmd);

    struct ofile *ofile = &ofiles[ostream];

    reset_pages(ostream);

    // Delete any file we created. Use the temp name if we have one. Note that
    // this needs to be done before closing the file, because that will delete
    // strings that we reference below.

    if (ofile->temp != NULL)
    {
        if (remove(ofile->temp) != 0)
        {
            throw(E_ERR, ofile->temp);
        }
    }
    else if (ofile->name != NULL)
    {
        if (remove(ofile->name) != 0)
        {
            throw(E_ERR, ofile->name);
        }
    }

    close_output(ostream);
}

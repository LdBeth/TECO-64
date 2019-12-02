///
///  @file    errors.h
///  @brief   TECO error messages
///
///  @author  Nowwith Treble Software
///
///  @bug     No known bugs.
///
///  @copyright  tbd
///
///  Permission is hereby granted, free of charge, to any person obtaining a copy
///  of this software and associated documentation files (the "Software"), to deal
///  in the Software without restriction, including without limitation the rights
///  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
///  copies of the Software, and to permit persons to whom the Software is
///  furnished to do so, subject to the following conditions:
///
///  The above copyright notice and this permission notice shall be included in
///  all copies or substantial portions of the Software.
///
///  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
///  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
///  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
///  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
///  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
///  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
///
////////////////////////////////////////////////////////////////////////////////

#if     !defined(_ERRORS_H)

#define _ERRORS_H

#include <stdnoreturn.h>

extern int last_error;

///  @enum   errors
///  @brief  Definitions of TECO error messages.

enum errors
{
    E_NUL = 1,      ///< No error
    E_ARG,          ///< Improper arguments
    E_BNI,          ///< >not in iteration
    E_CCL,          ///< CCV.SV not found or EG argument too long
    E_CHR,          ///< Invalid character for command
    E_CON,          ///< Confused use of conditionals
    E_CPQ,          ///< Can't pop into Q-register
    E_DEV,          ///< Invalid device
    E_DIV,          ///< Division by zero
    E_DTB,          ///< Delete too big
    E_ERR,          ///< RSTS/E error message
    E_FER,          ///< File error
    E_FNF,          ///< File not found "filespec"
    E_FUL,          ///< Output command would have overflowed output device
    E_ICE,          ///< Illegal ^E command in search argument
    E_IEC,          ///< Illegal character "x" after E
    E_IFC,          ///< Illegal character "x" after F
    E_IFE,          ///< Ill-formed numeric expression
    E_IFN,          ///< Illegal character "x" in filename
    E_IIA,          ///< Illegal insert arg
    E_ILL,          ///< Illegal command "x"
    E_ILN,          ///< Illegal number
    E_INP,          ///< Input error
    E_IPA,          ///< Negative or 0 argument to P
    E_IQC,          ///< Illegal " character
    E_IQN,          ///< Illegal Q-register name "x"
    E_IRA,          ///< Illegal radix argument to ^R
    E_ISA,          ///< Illegal search argument
    E_ISS,          ///< Illegal search string
    E_IUC,          ///< Illegal character "x" following ^
    E_MAP,          ///< Missing '
    E_MEM,          ///< Memory overflow
    E_MLA,          ///< Missing left angle bracket
    E_MLP,          ///< Missing (
    E_MOD,          ///< Invalid modifier
    E_MRA,          ///< Missing right angle bracket
    E_MRP,          ///< Missing )
    E_MSC,          ///< Missing start of conditional
    E_NAB,          ///< No arg before ^_
    E_NAC,          ///< No arg before ,
    E_NAE,          ///< No arg before =
    E_NAP,          ///< No arg before )
    E_NAQ,          ///< No arg before "
    E_NAS,          ///< No arg before ;
    E_NAU,          ///< No arg before U
    E_NCA,          ///< Negative argument to ,
    E_NFI,          ///< No file for input
    E_NFO,          ///< No file for output
    E_NIH,          ///< Not implemented here
    E_NPA,          ///< Negative or 0 argument to P
    E_NRO,          ///< No room for output
    E_NTF,          ///< No tag found
    E_NYA,          ///< Numeric argument with Y
    E_NYI,          ///< Not yet implemented
    E_OFO,          ///< Output file already open
    E_OUT,          ///< Output error
    E_PDO,          ///< Push-down list overflow
    E_PES,          ///< Attempt to pop empty stack
    E_POP,          ///< Attempt to move pointer off page with "x"
    E_SNI,          ///< ; not in iteration
    E_SRH,          ///< Search failure "text"
    E_STL,          ///< String too long
    E_SYS,          ///< System error message
    E_TAG,          ///< Missing tag !x!
    E_T10,          ///< TECO-10 command not implemented
    E_T32,          ///< TECO-32 command not implemented
    E_UCD,          ///< Unable to close and delete output file "x"
    E_UCI,          ///< Unable to close input file
    E_UCO,          ///< Unable to close output file
    E_UFI,          ///< Unable to open file "x" for input
    E_UFO,          ///< Unable to open file "x" for output
    E_UIT,          ///< Unable to initialize terminal
    E_URC,          ///< Unable to read character from terminal
    E_URE,          ///< Unable to read TECO command file
    E_URL,          ///< Unable to read line from input file
    E_UTC,          ///< Unterminated command "x"
    E_UTM,          ///< Unterminated macro
    E_UWL,          ///< Unable to write line to output file
    E_UWC,          ///< Unable to write character to output file
    E_WLO,          ///< System device write-locked
    E_XAB,          ///< Execution aborted
    E_YCA           ///< Y command aborted
};

extern noreturn void fatal_err(int err_sys, int err_teco, const char *err_arg);

extern void help_err(int err_teco);

extern noreturn void print_err(int err_teco);

extern noreturn void printc_err(int err_teco, int c);

extern noreturn void prints_err(int err_teco, const char *str);

#endif  // !defined(_ERRORS_H)

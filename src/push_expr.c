///
///  @file    push_expr.c
///  @brief   Push operand or operator on expression stack.
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

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "teco.h"
#include "errors.h"
#include "exec.h"

///  @struct  estack
///  @brief   Expression stack used for parsing command strings.

struct estack estack;

// Local functions

static void reduce(void);

static bool reduce2(void);

static bool reduce3(void);


///
///  @brief    Handle numeric argument from expression stack. We check to see
///            if there's something on the stack, and that the top element is
///            an operand. If so, we pop it off and return it whenever someone
///            needs a numeric value.
///
///  @returns  Value of argument (if error, returns to main loop).
///
////////////////////////////////////////////////////////////////////////////////

int get_n_arg(void)
{
    assert(estack.level > 0);           // Caller should check before calling

    --estack.level;

    if (estack.level == 0 && estack.obj[estack.level].type == EXPR_MINUS)
    {
        return -1;
    }

    if (estack.obj[estack.level].type != EXPR_VALUE)
    {
        print_err(E_IFE);               // Ill-formed numeric expression
    }

    return (int)estack.obj[estack.level].value;
}


///
///  @brief    Initialize expression stack.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

void init_expr(void)
{
    estack.level = 0;

    for (int i = 0; i < EXPR_SIZE; ++i)
    {
        estack.obj[i].type = estack.obj[i].value = 0;
    }
}


///
///  @brief    Return whether the top of the expression stack is an operand.
///            Note: if the stack is empty, then there's obviously no operand.
///
///  @returns  true if an operand, else false.
///
////////////////////////////////////////////////////////////////////////////////

bool operand_expr(void)
{
    if (estack.level == 0)              // Anything on stack?
    {
        return false;                   // No
    }

    if (estack.obj[estack.level - 1].type == EXPR_VALUE)
    {
        return true;                    // Done if we have an operand
    }

    // Say we have an "operand" if there is only one thing on the stack, and
    // it's a unary operator.

    if (estack.level == 1
        && estack.obj[0].type == EXPR_VALUE
        && estack.obj[0].value == '-')
    {
        return true;
    }

    return false;
}


///
///  @brief    Push operator or operand on expression stack.
///
///            This function pushes an value onto the expression stack. The
///            expression stack implement's TECO's expression handling capa-
///            bility. For instance, if a command like 10+qa=$ is executed,
///            then three values are pushed onto the expression stack: 10,
///            the plus sign and the value of qa. Each time a value is pushed
///            onto the expression stack, the reduce() function is called
///            to see if the stack can be reduced. In the above example, re-
///            duce() would cause the stack to be reduced when the value of
///            qa is pushed, because the expression can be evaluated then.
///
///  @returns  Nothing (return to main loop if error).
///
////////////////////////////////////////////////////////////////////////////////

void push_expr(int value, enum expr_type type)
{
    if (estack.level == EXPR_SIZE)
    {
        print_err(E_PDO);               // Push-down list overflow
    }

    scan_state = SCAN_EXPR;

    estack.obj[estack.level].value = value;
    estack.obj[estack.level].type = type;

    ++estack.level;

    reduce();                           // Reduce what we can
}


///
///  @brief    Reduce expression stack if possible.
///
///  @returns  Nothing.
///
////////////////////////////////////////////////////////////////////////////////

static void reduce(void)
{
    while (estack.level > 1)
    {
        if (!reduce3() && !reduce2())
        {
            break;
        }
    }
}


///
///  @brief    Reduce top two items on expression stack if possible.
///
///  @returns  true if we reduced expression stack, else false.
///
////////////////////////////////////////////////////////////////////////////////

static bool reduce2(void)
{
    if (estack.level < 2)               // At least two items?
    {
        return false;                   // No
    }

    struct e_obj *e1 = &estack.obj[estack.level - 1];
    struct e_obj *e2 = &estack.obj[estack.level - 2];

    if (e1->type == EXPR_VALUE && e2->type != EXPR_VALUE)
    {
        if (e2->type == '+')
        {
            e2->value = e1->value;
            e2->type = EXPR_VALUE;

            --estack.level;
        }
        else if (e2->type == '-')
        {
            e2->value = -e1->value;
            e2->type = EXPR_VALUE;

            --estack.level;
        }
        else
        {
            return false;
        }
    }
    else if (e1->type == (enum expr_type)'\x1F') // One's complement
    {
        if (estack.level == 1 || e2->type != EXPR_VALUE)
        {
            print_err(E_NAB);           // No argument before ^_
        }

        e2->value = (int)(~(uint)e2->value);

        --estack.level;
    }
    else
    {
        return false;
    }

    return true;
}

///
///  @brief    Reduce top three items on expression stack if possible.
///
///  @returns  true if we were able to reduce expression stack, else false.
///
////////////////////////////////////////////////////////////////////////////////

static bool reduce3(void)
{
    if (estack.level < 3)               // At least three items?
    {
        return false;                   // No
    }

    struct e_obj *e1 = &estack.obj[estack.level - 1];
    struct e_obj *e2 = &estack.obj[estack.level - 2];
    struct e_obj *e3 = &estack.obj[estack.level - 3];

    // Reduce (x) to x

    if (e3->type == '(' && e2->type == EXPR_VALUE && e1->type == ')')
    {
        e3->value = e2->value;
        e3->type = EXPR_VALUE;

        estack.level -= 2;

        return true;
    }

    // Anything else has to be of the form x <operator> y.

    if (e3->type != EXPR_VALUE ||
        e2->type == EXPR_VALUE ||
        e1->type != EXPR_VALUE)
    {
        return false;
    }

    // Here to process arithmetic and logical operators
    
    switch ((int)e2->type)
    {
        case '+':
            e3->value += e1->value;

            break;

        case '-':
            e3->value -= e1->value;

            break;

        case '*':
            e3->value *= e1->value;

            break;

        case '/':
            if (e1->value == 0)         // Division by zero?
            {
                // Don't allow divide by zero if we're scanning expression.

                if (scan_state == SCAN_EXPR)
                {
                    e1->value = 1;      // Just use a dummy result here
                }
                else
                {
                    print_err(E_DIV);
                }
            }

            e3->value /= e1->value;

            break;

        case '&':
            e3->value &= e1->value;

            break;

        case '#':
            e3->value |= e1->value;

            break;

        default:
            print_err(E_ARG);           // Improper arguments
    }

    estack.level -= 2;
    e3->type = EXPR_VALUE;

    return true;
}

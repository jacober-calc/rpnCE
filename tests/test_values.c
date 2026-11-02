/* Independent complex references plus whole-value dispatch and failure tests. */
#include <complex.h>
#include <stdio.h>
#include <string.h>
#include "calc.h"
#include "value_math.h"
#include "value_format.h"
#include "format.h"
#include "mock_calc_runtime.h"

static int fails, checks;
static calc_t c;
static void check(const char *name, bool pass)
{
    checks++;
    if (!pass) { fails++; printf("FAIL %s\n", name); }
}
static bool same(const value_t *v, double complex z)
{
    return !v->unit && (v->type == VALUE_REAL || v->type == VALUE_COMPLEX) &&
        cabs((v->real.v + (v->type == VALUE_COMPLEX ? v->imag.v : 0) * I) - z) <= 3e-11 * fmax(1, cabs(z));
}
static value_t z(double re, double im) { return value_complex(mk(re), mk(im)); }
static value_result_t eval(calc_cmd_t cmd, value_t y, value_t x)
{
    value_context_t ctx = {0};
    value_result_t result = {0};
    const char *err = value_eval(&ctx, cmd, &y, &x, &result);
    check("complex evaluation succeeds", err == NULL);
    return result;
}
static void refused(calc_cmd_t cmd, uint8_t arg, const char *error)
{
    calc_t before = c;
    calc_execute(&c, cmd, arg);
    check("failure returns the intended message", c.message && !strcmp(c.message, error));
    check("failure preserves complete operands, LASTX, registers and lift",
        !memcmp(c.stack, before.stack, sizeof c.stack) &&
        !memcmp(&c.lastx, &before.lastx, sizeof c.lastx) &&
        !memcmp(c.regs, before.regs, sizeof c.regs) &&
        !memcmp(&c.stats, &before.stats, sizeof c.stats) && c.lift == before.lift);
}
static void arithmetic(void)
{
    const double parts[] = {-7, -1, 0, .25, 3, 8};
    for (unsigned i = 0; i < 6; i++) for (unsigned j = 0; j < 6; j++) {
        double complex a = parts[i] + parts[j] * I;
        value_t va = z(parts[i], parts[j]);
        value_result_t r = eval(CMD_SQRT, va, va);
        check("principal square root across all quadrants", same(&r.x, csqrt(a)));
        if (cabs(a)) {
            r = eval(CMD_LN, va, va);
            check("principal logarithm across all quadrants", same(&r.x, clog(a)));
        }
        for (unsigned k = 0; k < 6; k++) {
            double complex b = parts[k] + parts[(k+2)%6] * I;
            value_t vb = z(creal(b), cimag(b));
            r = eval(CMD_ADD, va, vb); check("addition", same(&r.x, a+b));
            r = eval(CMD_SUB, va, vb); check("subtraction", same(&r.x, a-b));
            r = eval(CMD_MUL, va, vb); check("multiplication", same(&r.x, a*b));
            r = eval(CMD_DIV, va, vb); check("division", same(&r.x, a/b));
            if (cabs(a)) {
                r = eval(CMD_POW, va, vb); check("principal power", same(&r.x, cpow(a,b)));

            }
        }
    }
    value_result_t r = eval(CMD_DIV, z(8e99, 8e99), z(8e99, -8e99));
    check("scaled division avoids overflowing squares", same(&r.x, I));
    r = eval(CMD_COMPLEX_ABS, z(0,0), z(3e90,4e90));
    check("scaled magnitude avoids overflowing squares", same(&r.x, 5e90));
    r = eval(CMD_SQRT, z(0,0), z(8e99,8e99));
    check("square root tolerates an out-of-range intermediate magnitude", same(&r.x, csqrt(8e99+8e99*I)));
    r = eval(CMD_LN, z(0,0), z(8e99,8e99));
    check("logarithm tolerates an out-of-range intermediate magnitude", same(&r.x, clog(8e99+8e99*I)));
    r = eval(CMD_MUL, z(0,1), z(0,1));
    check("explicit complex arithmetic retains its type when imaginary part is zero", r.x.type == VALUE_COMPLEX && same(&r.x,-1));
}
static void stack_registers(void)
{
    calc_init(&c);
    c.stack[1] = value_real(mk(3)); c.stack[0] = value_real(mk(4));
    calc_execute(&c,CMD_COMPLEX_LINK,0);
    check("LINK consumes two levels and saves LASTX", same(&c.stack[0],3+4*I) && c.stack[1].real.v == 0 && same(&c.lastx,4));
    calc_execute(&c,CMD_STO,REG_A);
    calc_execute(&c,CMD_ENTER,0);
    check("ENTER duplicates a complete complex value", same(&c.stack[0],3+4*I) && same(&c.stack[1],3+4*I));
    calc_execute(&c,CMD_MUL,0);
    check("stack multiplication", same(&c.stack[0],-7+24*I));
    calc_execute(&c,CMD_LASTX,0);
    check("LASTX recalls both components", same(&c.stack[0],3+4*I));
    c.stack[0] = value_real(mk(2));
    calc_execute(&c,CMD_STO_MUL,REG_A);
    check("STO arithmetic shares mixed complex arithmetic", same(&c.regs[0],6+8*I) && same(&c.stack[0],2));
    calc_execute(&c,CMD_RCL_DIV,REG_A);
    check("RCL arithmetic preserves operand order", same(&c.stack[0],.12-.16*I));
    calc_execute(&c,CMD_XCH,REG_A);
    check("exchange moves complete values", same(&c.stack[0],6+8*I) && same(&c.regs[0],.12-.16*I));
    calc_execute(&c,CMD_COMPLEX_LINK,0);
    check("unlink leaves real Y and imaginary X", same(&c.stack[1],6) && same(&c.stack[0],8) && same(&c.lastx,6+8*I));
    c.stack[0] = z(3,4); c.stack[1] = value_real(mk(3)); c.stack[1].unit=1;
    refused(CMD_ADD,0,"Err: Data Type");
    refused(CMD_UNIT,0,"Err: Data Type");
    refused(CMD_SIN,0,"Err: Data Type");
    refused(CMD_STAT_PLUS,0,"Err: Data Type");
    refused(CMD_STO,REG_STAT_FIRST,"Err: Data Type");
    refused(CMD_XCH,REG_STAT_FIRST,"Err: Data Type");
    refused(CMD_TVM_VAR,TVM_PV,"Err: Data Type");
    refused(CMD_SET_STACK_SIZE,0,"Err: Data Type");
    refused(CMD_DATE_FN,DF_S_TO_TIME,"Err: Data Type");
    c.regs[REG_I-1]=z(1,0);
    refused(CMD_RCL,REG_INDIRECT,"Err: Data Type");
    calc_init(&c); c.base.radix=16; c.base.bits=8; c.lift=false;
    c.stack[0]=z(1,2); c.stack[1]=value_real(mk(3));
    calc_execute(&c,CMD_DIGIT,15); calc_execute(&c,CMD_DIGIT,15); calc_execute(&c,CMD_ADD,0);
    check("BASE dispatch uses the completed entry's type", same(&c.stack[0],2));
    c.stack[0]=value_real(mk(1)); c.stack[1]=z(1,2); calc_execute(&c,CMD_CHS,0);
    check("unary BASE operations ignore unrelated complex Y", same(&c.stack[0],255));
    calc_init(&c);
    c.stack[0]=z(0,0); c.stack[1]=z(1,2);
    refused(CMD_DIV,0,"Err: Division By 0");
    c.stack[0]=z(9e99,9e99);
    refused(CMD_COMPLEX_ABS,0,"Err: Overflow");
    c.stack[1]=z(1e-99,1); c.stack[0]=z(0.1,0);
    refused(CMD_MUL,0,"Err: Underflow");
    c.stack[1]=z(1,1e-99); c.stack[0]=z(10,0);
    refused(CMD_DIV,0,"Err: Underflow");
    calc_init(&c); c.stack[0]=value_real(mk(-4));
    refused(CMD_SQRT,0,"Err: Domain");
    calc_execute(&c,CMD_RESULT_COMPLEX,0); calc_execute(&c,CMD_SQRT,0);
    check("CRES permits a complex result from real operands", same(&c.stack[0],2*I));
    c.stack[0]=value_real(mk(4));
    refused(CMD_DATE_FN,DF_SHRINK,"Err: Data Type");
    refused(CMD_DATE_FN,DF_EXTEND,"Err: Data Type");
    refused(CMD_DATE_FN,DF_CUT,"Err: Data Type");
    refused(CMD_DATE_FN,DF_LINK,"Err: Data Type");
    refused(CMD_DATE_FN,DF_CVTZ,"Err: Data Type");
    calc_execute(&c,CMD_SQRT,0);
    check("ordinary real results remain real", c.stack[0].type==VALUE_REAL && same(&c.stack[0],2));
    calc_execute(&c,CMD_RESULT_REAL,0); c.stack[0]=z(0,1); calc_execute(&c,CMD_SQUARE,0);
    check("RRES still accepts explicit complex operands", same(&c.stack[0],-1) && c.stack[0].type==VALUE_COMPLEX);
}
static void input_modes_format(void)
{
    calc_init(&c);
    calc_execute(&c,CMD_DIGIT,3); calc_execute(&c,CMD_COMPLEX_RECT_ENTRY,0);
    calc_execute(&c,CMD_DIGIT,4); calc_execute(&c,CMD_ENTER,0);
    check("rectangular entry", same(&c.stack[0],3+4*I));
    calc_execute(&c,CMD_CLEAR,0); calc_execute(&c,CMD_COMPLEX_RECT_ENTRY,0); calc_finish_entry(&c);
    check("solitary i is the imaginary unit", same(&c.stack[0],I));
    calc_execute(&c,CMD_CLEAR,0); calc_execute(&c,CMD_DIGIT,2);
    calc_execute(&c,CMD_COMPLEX_POLAR_ENTRY,0); calc_execute(&c,CMD_DIGIT,9); calc_execute(&c,CMD_DIGIT,0);
    calc_execute(&c,CMD_ENTER,0); check("polar degree entry", same(&c.stack[0],2*I));
    calc_execute(&c,CMD_CLEAR,0); calc_execute(&c,CMD_DIGIT,2);
    calc_execute(&c,CMD_COMPLEX_POLAR_ENTRY,0); calc_execute(&c,CMD_COMPLEX_POLAR_ENTRY,0);
    calc_execute(&c,CMD_DIGIT,0); calc_execute(&c,CMD_ENTER,0);
    check("repeated ANGLE selects radians", same(&c.stack[0],2));
    calc_execute(&c,CMD_CLEAR,0); calc_execute(&c,CMD_DIGIT,1); calc_execute(&c,CMD_EE,0);
    calc_execute(&c,CMD_DIGIT,2); calc_execute(&c,CMD_COMPLEX_RECT_ENTRY,0);
    calc_execute(&c,CMD_DIGIT,3); calc_execute(&c,CMD_EE,0); calc_execute(&c,CMD_CHS,0); calc_execute(&c,CMD_DIGIT,2);
    calc_finish_entry(&c); check("component exponents and signs are independent", same(&c.stack[0],100+.03*I));
    c.stack[0]=z(3,4); c.complex_mode=COMPLEX_DEG; c.degrees=false;
    calc_execute(&c,CMD_COMPLEX_ANGLE,0); check("CANG follows trig radians independently of polar display", same(&c.stack[0],atan2(4,3)));
    c.stack[0]=z(3,4); c.degrees=true; calc_execute(&c,CMD_COMPLEX_ANGLE,0);
    check("CANG follows trig degrees", same(&c.stack[0],atan2(4,3)*180/M_PI));
    c.stack[0]=value_real(mk(-4)); calc_execute(&c,CMD_COMPLEX_ANGLE,0);
    check("CANG of a real matches the assembly's zero convention", same(&c.stack[0],0));
    c.stack[0]=z(3,4); c.complex_mode=COMPLEX_RECT;
    char text[VALUE_FORMAT_MAX];
    value_format(text,&c.stack[0],DISP_FIX,DISP_FLOAT,COMPLEX_RECT,false,false,true);
    check("SHOW formats both components", !strcmp(text,"3. i 4."));
    value_format(text,&c.stack[0],DISP_FIX,2,COMPLEX_DEG,false,false,false);
    check("polar degree formatting", !strcmp(text,"5.00 <d 53.13"));
    c.stack[0]=z(1234.5,-6789.25);
    value_format(text,&c.stack[0],DISP_FIX,2,COMPLEX_RECT,true,false,false);
    check("complex decimal comma formatting", !strcmp(text,"1.234,50 i -6.789,25"));
    value_format_entry(text,"1234.5i-6789.25",true,false);
    check("complex entry groups and localizes both components", !strcmp(text,"1.234,5 i -6.789,25"));
    value_format_entry(text,"2d90",false,false);
    check("polar entry identifies degrees", !strcmp(text,"2 <d 90"));
}
static void evaluation_boundaries(void)
{
    value_context_t ctx = {0};
    value_result_t result;
    value_t invalid = { .type = 255 }, nine = value_real(mk(9));
    const char *error = value_eval(&ctx, CMD_PI, &invalid, &invalid, &result);
    check("constants ignore unrelated invalid stack values", !error && same(&result.x, M_PI));
    error = value_eval(&ctx, CMD_SQRT, &invalid, &nine, &result);
    check("unary evaluation ignores invalid Y", !error && same(&result.x, 3) && !result.drop);
    error = value_eval(&ctx, CMD_ADD, &invalid, &nine, &result);
    check("binary evaluation validates Y", error && !strcmp(error, "Err: Data Type"));
    value_t number = z(3,4);
    error = value_eval(&ctx, CMD_COMPLEX_LINK, &invalid, &number, &result);
    check("complex LINK splits without reading unrelated Y", !error && result.split &&
          !result.drop && same(&result.y,3) && same(&result.x,4));
    ctx.degrees = true;
    error = value_eval(&ctx, CMD_COMPLEX_ANGLE, &invalid, &number, &result);
    check("angle context selects degrees", !error && same(&result.x,atan2(4,3)*180/M_PI));
    ctx.complex_results = true;
    value_t negative = value_real(mk(-1));
    error = value_eval(&ctx, CMD_LN, &invalid, &negative, &result);
    check("complex logarithm keeps radians inside a degree context", !error && same(&result.x,I*M_PI));
    error = value_eval(&ctx, CMD_COMPLEX_ANGLE, &invalid, &number, &result);
    check("logarithm evaluation leaves the angle context intact", !error && same(&result.x,atan2(4,3)*180/M_PI));

    calc_init(&c);
    for (int i = 1; i <= 3; i++) {
        c.stack[0] = value_real(mk(i)); c.stack[1] = value_real(mk(i*2));
        calc_execute(&c,CMD_STAT_PLUS,0);
    }
    c.stack[0] = value_real(mk(1e99));
    refused(CMD_SQUARE,0,"Err: Overflow");
    calc_t before = c;
    calc_execute(&c,CMD_FIT_BEST,0);
    check("model selection cannot reuse a failed arithmetic result", c.message && !strcmp(c.message,"LINF"));
    check("model selection preserves stack and LASTX after an error",
          !memcmp(c.stack,before.stack,sizeof c.stack) && !memcmp(&c.lastx,&before.lastx,sizeof c.lastx));
}

int main(void)
{
    value_math_init(); arithmetic(); stack_registers(); input_modes_format(); evaluation_boundaries();
    printf("%d value checks\n%d failures\n",checks,fails);
    return fails != 0;
}

#ifndef RPNCE_CORE_DATEOBJ_H
#define RPNCE_CORE_DATEOBJ_H

/*
 * DATE objects inside the calculator's number slots. A real number is 9
 * bytes: a sign byte (0x00 or 0x80 for a real) and 8 bytes of exponent and
 * digits. An object uses a sign byte real numbers never have (0x60 + its
 * type) and packs its fields into the other 8 bytes, so the stack,
 * registers, LASTX and the save file carry objects with no extra storage.
 * Real math must never see one: callers check obj_is() first.
 */

#include <stdbool.h>
#include <ti/real.h>
#include "datetime.h"

#define OBJ_MARK 0x60

static inline bool obj_is(const real_t *v)
{
    return ((uint8_t)v->sign & OBJ_MARK) == OBJ_MARK;
}

static inline real_t obj_pack(const dt_obj_t *o)
{
    real_t r;
    int q = o->tz_hour * 4 + o->tz_minute / 15;     /* -95 .. 95 */
    r.sign = (int8_t)(OBJ_MARK | o->type);
    r.exp = (int8_t)(o->year & 0xFF);
    r.mant[0] = (uint8_t)((uint16_t)o->year >> 8);
    r.mant[1] = (uint8_t)o->month;
    r.mant[2] = (uint8_t)o->day;
    r.mant[3] = (uint8_t)o->hour;
    r.mant[4] = (uint8_t)o->minute;
    r.mant[5] = (uint8_t)o->second;
    r.mant[6] = (uint8_t)(int8_t)q;
    return r;
}

static inline dt_obj_t obj_unpack(const real_t *r)
{
    dt_obj_t o;
    int q = (int8_t)r->mant[6];
    o.type = (uint8_t)r->sign & 0x0F;
    o.year = (int16_t)((uint16_t)(uint8_t)r->exp | ((uint16_t)r->mant[0] << 8));
    o.month = (int8_t)r->mant[1];
    o.day = (int8_t)r->mant[2];
    o.hour = (int8_t)r->mant[3];
    o.minute = (int8_t)r->mant[4];
    o.second = (int8_t)r->mant[5];
    o.tz_hour = (int8_t)(q / 4);
    o.tz_minute = (int8_t)((q % 4) * 15);
    return o;
}

#endif

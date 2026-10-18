#include <string.h>
#include <fileioc.h>

#include "res.h"
#include "res_info.h"
#include "../core/units.h"

/* Header and section offsets, as written by tools/genres.py. */
typedef struct {
    char magic[4];
    uint16_t format;
    uint16_t node_count;
    uint8_t group_count;
    uint8_t unit_count;
    uint16_t id_count;
    uint32_t stamp;
    uint16_t nodes, groups, parents, parent_rows, units, unit_base, unit_map, text;
} res_header_t;

typedef struct {
    uint16_t label;
    uint8_t dim;
    uint8_t reciprocal;
    uint16_t factor;
    uint16_t offset;
} unit_rec_t;

static const uint8_t *g_res;            /* the appvar's contents */
static const res_header_t *g_hdr;
static const char *g_problem = "not opened";

bool res_open(void)
{
    uint8_t h = ti_Open(RES_APPVAR, "r");
    if (!h) {
        g_problem = "RPNCEres not found";
        return false;
    }
    size_t size = ti_GetSize(h);
    const uint8_t *data = ti_GetDataPtr(h);
    ti_Close(h);                        /* the pointer stays good */

    const res_header_t *hd = (const res_header_t *)data;
    if (size < sizeof *hd || memcmp(hd->magic, "RPNR", 4) != 0) {
        g_problem = "RPNCEres is damaged";
        return false;
    }
    if (hd->format != RES_FORMAT || hd->stamp != RES_STAMP || size != RES_SIZE ||
        hd->node_count != MENU_NODE_COUNT || hd->group_count != MENU_GROUP_COUNT ||
        hd->unit_count != UNIT_COUNT || hd->id_count != MID_COUNT) {
        g_problem = "RPNCEres is the wrong version";
        return false;
    }
    g_res = data;
    g_hdr = hd;
    g_problem = NULL;
    return true;
}

const char *res_problem(void)
{
    return g_problem;
}

const char *res_text(uint16_t offset)
{
    return (const char *)g_res + g_hdr->text + offset;
}

const menu_node_t *menu_node(uint16_t index)
{
    return (const menu_node_t *)(g_res + g_hdr->nodes) + index;
}

const menu_group_t *menu_group(uint8_t group)
{
    return (const menu_group_t *)(g_res + g_hdr->groups) + group;
}

uint8_t menu_parent_of(uint8_t group)
{
    return g_res[g_hdr->parents + group];
}

uint8_t menu_parent_row_of(uint8_t group)
{
    return g_res[g_hdr->parent_rows + group];
}

int unit_for_menu(menu_id_t id)
{
    uint8_t u = g_res[g_hdr->unit_map + id];
    return u == 0xFF ? -1 : u;
}

/* ---------- the unit table (declared in core/units.h) ---------- */

static const unit_rec_t *unit_rec(uint8_t u)
{
    return (const unit_rec_t *)(g_res + g_hdr->units) + u;
}

const char *unit_label(uint8_t u)     { return res_text(unit_rec(u)->label); }
uint8_t unit_dim(uint8_t u)           { return unit_rec(u)->dim; }
const char *unit_factor(uint8_t u)    { return res_text(unit_rec(u)->factor); }
bool unit_reciprocal(uint8_t u)       { return unit_rec(u)->reciprocal != 0; }
uint8_t unit_base_of(uint8_t dim)     { return g_res[g_hdr->unit_base + dim]; }

const char *unit_offset(uint8_t u)
{
    uint16_t o = unit_rec(u)->offset;
    return o == 0xFFFF ? NULL : res_text(o);
}

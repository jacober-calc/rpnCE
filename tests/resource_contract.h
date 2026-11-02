#ifndef RPNCE_TEST_RESOURCE_CONTRACT_H
#define RPNCE_TEST_RESOURCE_CONTRACT_H

#include "res.h"
#include "units.h"
#include "actions.h"

#define RESOURCE_CONTRACT_EXPECTED UINT32_C(0x9345CE20)

static int resource_contract_unit(menu_id_t id)
{
    uint8_t arg;
    return action_for(id, false, &arg) == CMD_UNIT ? arg : -1;
}

/* Hash decoded contents and navigation, independent of offsets, packing or
 * compiler integer widths. Covers menu labels, units and HELP contents. */
static uint32_t resource_contract_hash = UINT32_C(2166136261);
static void resource_contract_number(uint16_t n)
{
    resource_contract_hash = (resource_contract_hash ^ (uint8_t)n) * UINT32_C(16777619);
    resource_contract_hash = (resource_contract_hash ^ (uint8_t)(n >> 8)) * UINT32_C(16777619);
}
static void resource_contract_text(const char *s)
{
    do {
        resource_contract_hash = (resource_contract_hash ^ (uint8_t)*s) * UINT32_C(16777619);
    } while (*s++);
}
static uint32_t resource_contract(void)
{
    resource_contract_hash = UINT32_C(2166136261);
    for (uint16_t i = 0; i < MENU_NODE_COUNT; i++) {
        const menu_node_t *n = menu_node(i);
        resource_contract_text(res_text(n->label));
        resource_contract_text(n->alt_label == UINT16_MAX ? "" : res_text(n->alt_label));
        resource_contract_number(n->id);
        menu_kind_t kind = menu_node_kind(n);
        resource_contract_number(kind);
        resource_contract_number(kind == MENU_GROUP ? n->group : 0);
    }
    for (uint8_t i = 0; i < MENU_GROUP_COUNT; i++) {
        const menu_group_t *g = menu_group(i);
        resource_contract_text(res_text(g->label));
        resource_contract_number(g->first);
        resource_contract_number(g->rows);
        resource_contract_number(menu_parent_of(i));
        resource_contract_number(menu_parent_row_of(i));
    }
    for (uint16_t i = 0; i < MID_COUNT; i++)
        resource_contract_number((uint16_t)resource_contract_unit((menu_id_t)i));
    for (uint8_t i = 0; i < UNIT_COUNT; i++) {
        resource_contract_text(unit_label(i));
        resource_contract_text(unit_factor(i));
        resource_contract_text(unit_offset(i) ? unit_offset(i) : "");
        resource_contract_number(unit_dim(i));
        resource_contract_number(unit_reciprocal(i));
    }
    for (uint8_t i = 0; i < DIM_COUNT; i++)
        resource_contract_number(unit_base_of(i));
    resource_contract_number(res_help_pages());
    for (uint8_t p = 0; p < res_help_pages(); p++) {
        resource_contract_number(res_help_lines(p));
        for (uint8_t l = 0; l < res_help_lines(p); l++)
            resource_contract_text(res_help_line(p, l));
    }
    return resource_contract_hash;
}

#endif

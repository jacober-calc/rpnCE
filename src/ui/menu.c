#include "menu.h"

void menu_home(menu_t *m)
{
    m->group = 0;
    m->row = 0;
    m->depth = 0;
}

void menu_row_up(menu_t *m)
{
    uint8_t rows = menu_groups[m->group].rows;
    m->row = (m->row == 0) ? rows - 1 : m->row - 1;
}

void menu_row_down(menu_t *m)
{
    uint8_t rows = menu_groups[m->group].rows;
    m->row = (m->row + 1 >= rows) ? 0 : m->row + 1;
}

void menu_back(menu_t *m)
{
    if (m->depth == 0)
        return;
    m->depth--;
    m->group = m->back[m->depth].group;
    m->row = m->back[m->depth].row;
}

const menu_node_t *menu_slot(const menu_t *m, uint8_t slot)
{
    const menu_group_t *g = &menu_groups[m->group];
    return &menu_nodes[g->first + m->row * 5 + slot];
}

const menu_node_t *menu_press(menu_t *m, uint8_t slot)
{
    const menu_node_t *n = menu_slot(m, slot);

    if (n->kind == MENU_BLANK)
        return NULL;

    if (n->kind == MENU_GROUP) {
        if (m->depth < MENU_MAX_DEPTH) {
            m->back[m->depth].group = m->group;
            m->back[m->depth].row = m->row;
            m->depth++;
        }
        m->group = n->group;
        m->row = 0;
        return NULL;
    }

    return n;
}

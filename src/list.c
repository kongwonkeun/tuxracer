/* 
 * Tux Racer 
 * Copyright (C) 1999-2001 Jasmin F. Patry
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "tuxracer.h"
#include "list.h"

/* Internal node type */
typedef struct node_ {
    struct node_ *next;
    struct node_ *prev;
    list_elem_data_t data;
}
node_t;

/* Internal list type */
typedef struct {
    node_t *head;
    node_t *tail;
    int count;
}
dllist_t;

list_t create_list(void)
{
    dllist_t *dllist;

    dllist = (dllist_t*)malloc(sizeof(dllist_t));

    if (dllist == NULL) handle_error(1, "malloc error");

    dllist->head  = NULL;
    dllist->tail  = NULL;
    dllist->count = 0;

    return (list_t)dllist;
}

void del_list(list_t llist)
{
    dllist_t *dllist;
    node_t *node;
    node_t *nextnode;

    dllist = (dllist_t*) llist;

    for ( node = dllist->head; node != NULL; node = nextnode ) {
        nextnode = node->next;
        free( node );
    }

    free( dllist );
}

list_elem_t get_list_head(list_t llist)
{
    dllist_t *dllist;
    dllist = (dllist_t*) llist;
    return (list_elem_t) dllist->head;
}

list_elem_t get_list_tail(list_t llist)
{
    dllist_t *dllist;
    dllist = (dllist_t*) llist;
    return (list_elem_t) dllist->tail;
}

list_elem_t get_next_list_elem(list_t llist, list_elem_t node)
{
    node_t *n;
    check_assertion( node != NULL, "node is null" );
    n = (node_t*) node;

    return (list_elem_t) n->next;
}

list_elem_t get_prev_list_elem(list_t llist, list_elem_t node)
{
    node_t *n;
    check_assertion( node != NULL, "node is null" );
    n = (node_t*) node;

    return (list_elem_t) n->prev;
}

list_elem_t insert_list_elem(list_t llist, list_elem_t node_before, list_elem_data_t new_data)
{
    dllist_t *dllist;
    node_t *prev_node;
    node_t **prev;
    node_t *new_node;
    node_t *next_node;
    node_t **next;

    check_assertion(llist != NULL, "llist is null");

    dllist = (dllist_t*)llist;
    prev_node = (node_t*)node_before;

    new_node = (node_t*)malloc(sizeof(node_t));
    new_node->data = new_data;

    if (prev_node == NULL) {
        next_node = dllist->head;
    } else {
        next_node = prev_node->next;
    }

    if (prev_node == NULL) {
        next = &(dllist->head);
    } else {
        next = &(prev_node->next);
    }

    if (next_node == NULL) {
        prev = &(dllist->tail);
    } else {
        prev = &(next_node->prev);
    }

    *next = new_node;
    *prev = new_node;
    new_node->prev = prev_node;
    new_node->next = next_node;

    dllist->count += 1;

    return (list_elem_t)new_node;
}

list_elem_data_t delete_list_elem(list_t llist, list_elem_t node)
{
    dllist_t *dllist;
    node_t *prev_node;
    node_t **prev;
    node_t *del_node;
    node_t *next_node;
    node_t **next;
    list_elem_data_t data;

    check_assertion( llist != NULL, "llist is null" );
    check_assertion( node != NULL, "node is null" );

    dllist = (dllist_t*) llist;
    del_node = (node_t*) node;

    data = del_node->data;

    prev_node = del_node->prev;
    next_node = del_node->next;

    if ( prev_node == NULL ) {
        next = &(dllist->head);
    } else {
        next = &(prev_node->next);
    }

    if ( next_node == NULL ) {
        prev = &(dllist->tail);
    } else {
        prev = &(next_node->prev);
    }

    *next = next_node;
    *prev = prev_node;

    free( del_node );

    dllist->count -= 1;

    check_assertion( dllist->count >= 0, "negative list size" );

    return data;
}

list_elem_data_t get_list_elem_data( list_elem_t node )
{
    node_t *n;

    check_assertion( node != NULL, "node is null" );

    n = (node_t*) node;

    return n->data;
}

void list_sanity_check( list_t llist )
{
    int size = 0;
    dllist_t *dllist = (dllist_t*) llist;
    node_t *node;

    for ( node = dllist->head; node != NULL; node = node->next )
    {
        if ( node->prev != NULL ) {
            check_assertion( node->prev->next == node, "invalid list" );
        } else {
            check_assertion( node == dllist->head, "invalid head ptr" );
        }
        if ( node->next != NULL ) {
            check_assertion( node->next->prev == node, "invalid list" );
        } else {
            check_assertion( node == dllist->tail, "invalid tail ptr" );
        }

        size += 1;
    }

    check_assertion( size == dllist->count, "list size incorrect" );
}

void print_list( list_t llist )
{
    int size = 0;
    dllist_t *dllist = (dllist_t*) llist;
    node_t *node;

    for ( node = dllist->head; node != NULL; node = node->next )
    {
        printf( "value of elem %d: %p\n", size, node->data ); //---- kong ---- %d to %p
        size += 1;
    }

    if ( size == 0 ) {
        printf( "list is empty\n" );
    }
    printf( "\n" );
}

/* EOF */
/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2006-03-16     Bernard      the first version
 * 2006-09-07     Bernard      move the kservice APIs to rtthread.h
 * 2007-06-27     Bernard      fix the ym_list_remove bug
 * 2012-03-22     Bernard      rename kservice.h to rtservice.h
 * 2017-11-15     JasonJia     Modify ym_slist_foreach to ym_slist_for_each_entry.
 *                             Make code cleanup.
 * 2023-07-06     zhaoyu.wu    migration to ym_xxx structure.
 */

#ifndef __YM_SERVICE_H__
#define __YM_SERVICE_H__

#include "ymdef.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup KernelService
 */

/**@{*/

/**
 * Double List structure
 */
struct ym_list_node {
    struct ym_list_node *next;         /**< point to next node. */
    struct ym_list_node *prev;         /**< point to prev node. */
};
typedef struct ym_list_node ym_list_t; /**< Type for lists. */

/**
 * Single List structure
 */
struct ym_slist_node {
    struct ym_slist_node *next;          /**< point to next node. */
};
typedef struct ym_slist_node ym_slist_t; /**< Type for single list. */

/**
 * ym_container_of - return the start address of struct type, while ptr is the
 * member of struct type.
 */
#define ym_container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

/**
 * @brief initialize a list object
 */
#define YM_LIST_OBJECT_INIT(object) \
    {                               \
        &(object), &(object)        \
    }

/**
 * @brief initialize a list
 *
 * @param l list to be initialized
 */
ym_inline void ym_list_init(ym_list_t *l)
{
    l->next = l->prev = l;
}

/**
 * @brief insert a node after a list
 *
 * @param l list to insert it
 * @param n new node to be inserted
 */
ym_inline void ym_list_inseym_after(ym_list_t *l, ym_list_t *n)
{
    l->next->prev = n;
    n->next       = l->next;

    l->next = n;
    n->prev = l;
}

/**
 * @brief insert a node before a list
 *
 * @param n new node to be inserted
 * @param l list to insert it
 */
ym_inline void ym_list_inseym_before(ym_list_t *l, ym_list_t *n)
{
    l->prev->next = n;
    n->prev       = l->prev;

    l->prev = n;
    n->next = l;
}

/**
 * @brief remove node from list.
 * @param n the node to remove from the list.
 */
ym_inline void ym_list_remove(ym_list_t *n)
{
    n->next->prev = n->prev;
    n->prev->next = n->next;

    n->next = n->prev = n;
}

/**
 * @brief tests whether a list is empty
 * @param l the list to test.
 */
ym_inline int ym_list_isempty(const ym_list_t *l)
{
    return l->next == l;
}

/**
 * @brief get the list length
 * @param l the list to get.
 */
ym_inline unsigned int ym_list_len(const ym_list_t *l)
{
    unsigned int     len = 0;
    const ym_list_t *p   = l;
    while (p->next != l) {
        p = p->next;
        len++;
    }

    return len;
}

/**
 * @brief get the struct for this entry
 * @param node the entry point
 * @param type the type of structure
 * @param member the name of list in structure
 */
#define ym_list_entry(node, type, member) \
    ym_container_of(node, type, member)

/**
 * ym_list_for_each - iterate over a list
 * @pos:    the ym_list_t * to use as a loop cursor.
 * @head:   the head for your list.
 */
#define ym_list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * ym_list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:    the ym_list_t * to use as a loop cursor.
 * @n:      another ym_list_t * to use as temporary storage
 * @head:   the head for your list.
 */
#define ym_list_for_each_safe(pos, n, head)                \
    for (pos = (head)->next, n = pos->next; pos != (head); \
         pos = n, n = pos->next)

/**
 * ym_list_for_each_entry  -   iterate over list of given type
 * @pos:    the type * to use as a loop cursor.
 * @head:   the head for your list.
 * @member: the name of the list_struct within the struct.
 */
#define ym_list_for_each_entry(pos, head, member)                 \
    for (pos = ym_list_entry((head)->next, typeof(*pos), member); \
         &pos->member != (head);                                  \
         pos = ym_list_entry(pos->member.next, typeof(*pos), member))

/**
 * ym_list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:    the type * to use as a loop cursor.
 * @n:      another type * to use as temporary storage
 * @head:   the head for your list.
 * @member: the name of the list_struct within the struct.
 */
#define ym_list_for_each_entry_safe(pos, n, head, member)             \
    for (pos = ym_list_entry((head)->next, typeof(*pos), member),     \
        n    = ym_list_entry(pos->member.next, typeof(*pos), member); \
         &pos->member != (head);                                      \
         pos = n, n = ym_list_entry(n->member.next, typeof(*n), member))

/**
 * ym_list_first_entry - get the first element from a list
 * @ptr:    the list head to take the element from.
 * @type:   the type of the struct this is embedded in.
 * @member: the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define ym_list_first_entry(ptr, type, member) \
    ym_list_entry((ptr)->next, type, member)

#define YM_SLIST_OBJECT_INIT(object) \
    {                                \
        YM_NULL                      \
    }

/**
 * @brief initialize a single list
 *
 * @param l the single list to be initialized
 */
ym_inline void ym_slist_init(ym_slist_t *l)
{
    l->next = YM_NULL;
}

ym_inline void ym_slist_append(ym_slist_t *l, ym_slist_t *n)
{
    struct ym_slist_node *node;

    node = l;
    while (node->next) node = node->next;

    /* append the node to the tail */
    node->next = n;
    n->next    = YM_NULL;
}

ym_inline void ym_slist_insert(ym_slist_t *l, ym_slist_t *n)
{
    n->next = l->next;
    l->next = n;
}

ym_inline unsigned int ym_slist_len(const ym_slist_t *l)
{
    unsigned int      len  = 0;
    const ym_slist_t *list = l->next;
    while (list != YM_NULL) {
        list = list->next;
        len++;
    }

    return len;
}

ym_inline ym_slist_t *ym_slist_remove(ym_slist_t *l, ym_slist_t *n)
{
    /* remove slist head */
    struct ym_slist_node *node = l;
    while (node->next && node->next != n) node = node->next;

    /* remove node */
    if (node->next != (ym_slist_t *)0) node->next = node->next->next;

    return l;
}

ym_inline ym_slist_t *ym_slist_first(ym_slist_t *l)
{
    return l->next;
}

ym_inline ym_slist_t *ym_slist_tail(ym_slist_t *l)
{
    while (l->next) l = l->next;

    return l;
}

ym_inline ym_slist_t *ym_slist_next(ym_slist_t *n)
{
    return n->next;
}

ym_inline int ym_slist_isempty(ym_slist_t *l)
{
    return l->next == YM_NULL;
}

/**
 * @brief get the struct for this single list node
 * @param node the entry point
 * @param type the type of structure
 * @param member the name of list in structure
 */
#define ym_slist_entry(node, type, member) \
    ym_container_of(node, type, member)

/**
 * ym_slist_for_each - iterate over a single list
 * @pos:    the ym_slist_t * to use as a loop cursor.
 * @head:   the head for your single list.
 */
#define ym_slist_for_each(pos, head) \
    for (pos = (head)->next; pos != YM_NULL; pos = pos->next)

/**
 * ym_slist_for_each_entry  -   iterate over single list of given type
 * @pos:    the type * to use as a loop cursor.
 * @head:   the head for your single list.
 * @member: the name of the list_struct within the struct.
 */
#define ym_slist_for_each_entry(pos, head, member)                 \
    for (pos = ym_slist_entry((head)->next, typeof(*pos), member); \
         &pos->member != (YM_NULL);                                \
         pos = ym_slist_entry(pos->member.next, typeof(*pos), member))

/**
 * ym_slist_first_entry - get the first element from a slist
 * @ptr:    the slist head to take the element from.
 * @type:   the type of the struct this is embedded in.
 * @member: the name of the slist_struct within the struct.
 *
 * Note, that slist is expected to be not empty.
 */
#define ym_slist_first_entry(ptr, type, member) \
    ym_slist_entry((ptr)->next, type, member)

/**
 * ym_slist_tail_entry - get the tail element from a slist
 * @ptr:    the slist head to take the element from.
 * @type:   the type of the struct this is embedded in.
 * @member: the name of the slist_struct within the struct.
 *
 * Note, that slist is expected to be not empty.
 */
#define ym_slist_tail_entry(ptr, type, member) \
    ym_slist_entry(ym_slist_tail(ptr), type, member)

/**@}*/

#ifdef __cplusplus
}
#endif

#endif

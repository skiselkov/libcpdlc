/*
 * Copyright 2019 Saso Kiselkov
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "cpdlc_assert.h"
#include "minilist.h"

#define	P2N(list, ptr) \
	((minilist_node_t *)((ptr) + ((list)->offset)))

void
minilist_create(minilist_t *list, size_t size, size_t offset)
{
	CPDLC_ASSERT(list != NULL);
	CPDLC_ASSERT(size != 0);
	CPDLC_ASSERT3U(size, >=, offset + sizeof (minilist_node_t));

	list->head = list->tail = NULL;
	list->size = size;
	list->offset = offset;
	list->count = 0;
}

void
minilist_destroy(minilist_t *list)
{
	CPDLC_ASSERT(list != NULL);
	CPDLC_ASSERT0(list->count);
	list->head = list->tail = NULL;
	list->size = 0;
	list->offset = 0;
}

void *
minilist_head(const minilist_t *list)
{
	CPDLC_ASSERT(list != NULL);
	return (list->head);
}

void *
minilist_tail(const minilist_t *list)
{
	CPDLC_ASSERT(list != NULL);
	return (list->tail);
}

void *
minilist_next(const minilist_t *list, const void *elem)
{
	CPDLC_ASSERT(list != NULL);
	CPDLC_ASSERT(elem != NULL);
	return (P2N(list, elem)->next);
}

void *
minilist_prev(const minilist_t *list, const void *elem)
{
	CPDLC_ASSERT(list != NULL);
	CPDLC_ASSERT(elem != NULL);
	return (P2N(list, elem)->prev);
}

void *
minilist_get_i(const minilist_t *list, unsigned idx)
{
	void *p;

	CPDLC_ASSERT(list != NULL);
	CPDLC_ASSERT(idx < list->count);
	p = minilist_head(list);
	for (; idx != 0; idx--)
		p = minilist_next(list, p);

	return (p);
}

size_t
minilist_count(const minilist_t *list)
{
	CPDLC_ASSERT(list != NULL);
	return (list->count);
}

void
minilist_insert_head(minilist_t *list, void *elem)
{
	CPDLC_ASSERT(list != NULL);
	CPDLC_ASSERT(elem != NULL);
	if (list->head != NULL) {
		/* List already populated, prepend */
		P2N(list, list->head)->prev = elem;
		P2N(list, elem)->prev = NULL;
		P2N(list, elem)->next = list->head;
		list->head = elem;
	} else {
		/* New insert */
		list->head = list->tail = elem;
		P2N(list, elem)->next = NULL;
		P2N(list, elem)->prev = NULL;
	}
	list->count++;
}

void
minilist_insert_tail(minilist_t *list, void *elem)
{
	CPDLC_ASSERT(list != NULL);
	CPDLC_ASSERT(elem != NULL);
	if (list->tail != NULL) {
		/* List already populated, append */
		P2N(list, list->tail)->next = elem;
		P2N(list, elem)->prev = list->tail;
		P2N(list, elem)->next = NULL;
		list->tail = elem;
	} else {
		/* New insert */
		list->head = list->tail = elem;
		P2N(list, elem)->next = NULL;
		P2N(list, elem)->prev = NULL;
	}
	list->count++;
}

void
minilist_remove(minilist_t *list, void *elem)
{
	void *next_elem, *prev_elem;

	CPDLC_ASSERT(list != NULL);
	CPDLC_ASSERT(elem != NULL);
	CPDLC_ASSERT(list->count != 0);

	next_elem = P2N(list, elem)->next;
	prev_elem = P2N(list, elem)->prev;
	if (list->head == elem)
		list->head = next_elem;
	if (list->tail == elem)
		list->tail = prev_elem;
	if (next_elem != NULL) {
		P2N(list, next_elem)->prev = prev_elem;
		P2N(list, elem)->next = NULL;
	}
	if (prev_elem != NULL) {
		P2N(list, prev_elem)->next = next_elem;
		P2N(list, elem)->prev = NULL;
	}
	list->count--;
}

static inline void
minilist_insert_between(minilist_t *list, void *elem, void *before, void *after)
{
	CPDLC_ASSERT(before != NULL);
	CPDLC_ASSERT(after != NULL);
	P2N(list, elem)->prev = before;
	P2N(list, elem)->next = after;
	P2N(list, before)->next = elem;
	P2N(list, after)->prev = elem;
}

void
minilist_insert_before(minilist_t *list, void *new_elem, void *old_elem)
{
	CPDLC_ASSERT(list != NULL);
	CPDLC_ASSERT(new_elem != NULL);
	CPDLC_ASSERT(old_elem != NULL);

	if (list->head == old_elem) {
		minilist_insert_head(list, new_elem);
	} else {
		minilist_insert_between(list, new_elem,
		    P2N(list, old_elem)->prev, old_elem);
		list->count++;
	}
}

void
minilist_insert_after(minilist_t *list, void *new_elem, void *old_elem)
{
	CPDLC_ASSERT(list != NULL);
	CPDLC_ASSERT(new_elem != NULL);
	CPDLC_ASSERT(old_elem != NULL);

	if (list->tail == old_elem) {
		minilist_insert_tail(list, new_elem);
	} else {
		minilist_insert_between(list, new_elem,
		    old_elem, P2N(list, old_elem)->next);
		list->count++;
	}
}

void *
minilist_remove_head(minilist_t *list)
{
	void *p;

	CPDLC_ASSERT(list != NULL);
	p = list->head;
	if (p != NULL)
		minilist_remove(list, p);
	return (p);
}

void *
minilist_remove_tail(minilist_t *list)
{
	void *p;

	CPDLC_ASSERT(list != NULL);
	p = list->tail;
	if (p != NULL)
		minilist_remove(list, p);
	return (p);
}

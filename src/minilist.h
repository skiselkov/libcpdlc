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

#ifndef	_LIBCPDLC_MINILIST_H_
#define	_LIBCPDLC_MINILIST_H_

#include <stdlib.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct {
	void	*next;
	void	*prev;
} list_node_t;

typedef struct {
	void	*head;
	void	*tail;
	size_t	size;
	size_t	offset;
	size_t	count;
} list_t;

void list_create(list_t *list, size_t size, size_t offset);
void list_destroy(list_t *list);
void *list_head(const list_t *list);
void *list_tail(const list_t *list);
void *list_next(const list_t *list, void *elem);
void *list_prev(const list_t *list, void *elem);
size_t list_count(const list_t *list);

void list_insert_head(list_t *list, void *elem);
void list_insert_tail(list_t *list, void *elem);
void list_remove(list_t *list, void *elem);
void list_insert_before(list_t *list, void *new_elem, void *old_elem);
void list_insert_after(list_t *list, void *new_elem, void *old_elem);

void *list_remove_head(list_t *list);
void *list_remove_tail(list_t *list);

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_MINILIST_H_ */

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
} minilist_node_t;

typedef struct {
	void	*head;
	void	*tail;
	size_t	size;
	size_t	offset;
	size_t	count;
} minilist_t;

void minilist_create(minilist_t *list, size_t size, size_t offset);
void minilist_destroy(minilist_t *list);
void *minilist_head(const minilist_t *list);
void *minilist_tail(const minilist_t *list);
void *minilist_next(const minilist_t *list, const void *elem);
void *minilist_prev(const minilist_t *list, const void *elem);
void *minilist_get_i(const minilist_t *list, unsigned idx);
size_t minilist_count(const minilist_t *list);

void minilist_insert_head(minilist_t *list, void *elem);
void minilist_insert_tail(minilist_t *list, void *elem);
void minilist_remove(minilist_t *list, void *elem);
void minilist_insert_before(minilist_t *list, void *new_elem, void *old_elem);
void minilist_insert_after(minilist_t *list, void *new_elem, void *old_elem);

void *minilist_remove_head(minilist_t *list);
void *minilist_remove_tail(minilist_t *list);

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_MINILIST_H_ */

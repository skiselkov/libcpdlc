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

#ifdef	__cplusplus
extern "C" {
#endif

#define	LIST_INSERT_HEAD(head, tail, new_elem) \
	do { \
		if ((head) != NULL) { \
			(head)->prev = (new_elem); \
			(new_elem)->next = (head); \
			(head) = (new_elem); \
		} else { \
			(head) = (tail) = (new_elem); \
		} \
	} while (0)

#define	LIST_INSERT_TAIL(head, tail, new_elem) \
	do { \
		if ((tail) != NULL) { \
			(tail)->next = (new_elem); \
			(new_elem)->prev = (tail); \
			(tail) = (new_elem); \
		} else { \
			(head) = (tail) = (new_elem); \
		} \
	} while (0)

#define	LIST_REMOVE(head, tail, rem_elem) \
	do { \
		if ((head) == (rem_elem)) \
			(head) = (rem_elem)->next; \
		if ((tail) == (rem_elem)) \
			(tail) = (rem_elem)->prev; \
		if ((rem_elem)->next != NULL) \
			(rem_elem)->next->prev = (rem_elem)->prev; \
		if ((rem_elem)->prev != NULL) \
			(rem_elem)->prev->next = (rem_elem)->next; \
	} while (0)

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_MINILIST_H_ */

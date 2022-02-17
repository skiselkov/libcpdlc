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

#ifndef	_LIBCPDLC_THREAD_H_
#define	_LIBCPDLC_THREAD_H_

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#if APL || LIN
#include <pthread.h>
#include <stdint.h>
#include <time.h>
#else	/* IBM */
#include <windows.h>
#endif	/* IBM */

#if	LIN
#include <sys/syscall.h>
#include <unistd.h>
#endif	/* LIN */

#include "cpdlc_alloc.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Basic portable multi-threading API. We have 3 kinds of objects and
 * associated manipulation functions here:
 * 1) threat_t - A generic thread identification structure.
 * 2) mutex_t - A generic mutual exclusion lock.
 * 3) condvar_t - A generic condition variable.
 *
 * The actual implementation is all contained in this header and simply
 * works as a set of macro expansions on top of the OS-specific threading
 * API (pthreads on Unix/Linux, winthreads on Win32).
 *
 * Example of how to create a thread:
 *	thread_t my_thread;
 *	if (!thread_create(&my_thread, thread_main_func, thread_arg))
 *		fprintf(stderr, "thread create failed!\n");
 *
 * Example of how to wait for a thread to exit:
 *	thread_t my_thread;
 *	thread_join(my_thread);
 *	... thread disposed of, no need for further cleanup ...
 *
 * Example of how to use a mutex_t:
 *	mutex_t my_lock;		-- the lock object itself
 *	mutex_init(&my_lock);		-- create a lock
 *	mutex_enter(&my_lock);		-- grab a lock
 *	... do some critical, exclusiony-type stuff ...
 *	mutex_exit(&my_lock);		-- release a lock
 *	mutex_destroy(&my_lock);	-- free a lock
 *
 * Example of how to use a condvar_t:
 *	mutex_t my_lock;
 *	condvar_t my_cv;
 *
 *	mutex_init(&my_lock);		-- create a lock to control the CV
 *	cv_init(&my_cv);		-- create the condition variable
 *
 *	-- thread that's going to signal the condition:
 *		mutex_enter(&my_lock);	-- grab the lock
 *		... set up some resource that others might be waiting on ...
 *		cv_broadcast(&my_lock);	-- wake up all waiters
 *		mutex_exit(&my_lock);	-- release the lock
 *
 *	-- thread that's going to wait on the condition:
 *		mutex_enter(&my_lock);			-- grab the lock
 *		while (!condition_met())
 *			cv_wait(&my_cv, &my_lock);	-- wait for the CV
 *							-- to be signalled
 *		... condition fulfilled, use the resource ...
 *		mutex_exit(&my_lock);			-- release the lock
 *
 * You can also performed a "timed" wait on a CV using cv_timedwait. The
 * function will exit when either the condition has been signalled, or the
 * timer has expired. The return value of the function indicates whether
 * the condition was signalled before the timer expired (returns zero),
 * or if the wait timed out (returns ETIMEDOUT) or another error occurred
 * (returns -1).
 *
 *		mutex_enter(&my_lock);			-- grab the lock
 *		-- Wait for the CV to be signalled. Time argument is an
 *		-- absolute time as returned by the 'microclock' function +
 *		-- whatever extra time delay you want to apply.
 *		uint64_t deadline = microclock() + timeout_usecs;
 *		while (!condition_met()) {
 *			if (cv_timedwait(&my_cv, &my_lock, deadline) ==
 *			    ETIMEDOUT) {
 *				-- timed out waiting for CV to signal
 *				break;
 *			}
 *		}
 *		mutex_exit(&my_lock);			-- release the lock
 */

typedef struct {
	void	(*proc)(void *);
	void	*arg;
} cpdlc_thread_info_t;

#if	APL || LIN

#define	thread_t		pthread_t
#define	thread_id_t		pthread_t
#define	mutex_t			pthread_mutex_t
#define	condvar_t		pthread_cond_t
#define	curthread		pthread_self()

#define	mutex_init(mtx)	\
	do { \
		pthread_mutexattr_t attr; \
		pthread_mutexattr_init(&attr); \
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE); \
		pthread_mutex_init((mtx), &attr); \
	} while (0)
#define	mutex_destroy(mtx)	pthread_mutex_destroy((mtx))
#define	mutex_enter(mtx)	pthread_mutex_lock((mtx))
#define	mutex_exit(mtx)		pthread_mutex_unlock((mtx))

static void *_cpdlc_thread_start_routine(void *arg) CPDLC_UNUSED_ATTR;
static void *
_cpdlc_thread_start_routine(void *arg)
{
	cpdlc_thread_info_t *ti = (cpdlc_thread_info_t *)arg;
	ti->proc(ti->arg);
	free(ti);
	return (NULL);
}

static inline bool
thread_create(thread_t *thrp, void (*proc)(void *), void *arg)
{
	cpdlc_thread_info_t *ti =
	    (cpdlc_thread_info_t *)safe_calloc(1, sizeof (*ti));
	ti->proc = proc;
	ti->arg = arg;
	if (pthread_create(thrp, NULL, _cpdlc_thread_start_routine, ti) == 0)
		return (true);
	free(ti);
	return (false);
}

#define	thread_join(thrp)	pthread_join(*(thrp), NULL)

#if	LIN
#define	thread_set_name(name)	pthread_setname_np(pthread_self(), (name))
#else	/* APL */
#define	thread_set_name(name)	pthread_setname_np((name))
#endif	/* APL */

#define	cv_wait(cv, mtx)	pthread_cond_wait((cv), (mtx))
static inline int
cv_timedwait(condvar_t *cv, mutex_t *mtx, uint64_t limit)
{
	struct timespec ts = { .tv_sec = (time_t)(limit / 1000000),
	    .tv_nsec = (long)((limit % 1000000) * 1000) };
	return (pthread_cond_timedwait(cv, mtx, &ts));
}
#define	cv_init(cv)		pthread_cond_init((cv), NULL)
#define	cv_destroy(cv)		pthread_cond_destroy((cv))
#define	cv_broadcast(cv)	pthread_cond_broadcast((cv))

static inline uint64_t
cpdlc_thread_microclock(void)
{
	struct timespec ts;
	CPDLC_VERIFY(clock_gettime(CLOCK_REALTIME, &ts) == 0);
	return ((ts.tv_sec * 1000000llu) + (ts.tv_nsec / 1000llu));
}

#define	cpdlc_usleep(x)	usleep(x)

#if	LIN
#define	CPDLC_VERIFY_MUTEX_HELD(mtx)	\
	CPDLC_VERIFY((mtx)->__data.__owner == syscall(SYS_gettid))
#define	CPDLC_VERIFY_MUTEX_NOT_HELD(mtx) \
	CPDLC_VERIFY((mtx)->__data.__owner != syscall(SYS_gettid))
#else	/* APL */
#define	CPDLC_VERIFY_MUTEX_HELD(mtx)		(void)1
#define	CPDLC_VERIFY_MUTEX_NOT_HELD(mtx)	(void)1
#endif	/* APL */

#else	/* IBM */

#define	thread_t	HANDLE
#define	thread_id_t	DWORD
typedef struct {
	BOOL			inited;
	CRITICAL_SECTION	cs;
} mutex_t;
#define	condvar_t	CONDITION_VARIABLE
#define	curthread	GetCurrentThreadId()

#define	mutex_init(x) \
	do { \
		(x)->inited = TRUE; \
		InitializeCriticalSection(&(x)->cs); \
	} while (0)
#define	mutex_destroy(x) \
	do { \
		CPDLC_ASSERT((x)->inited); \
		DeleteCriticalSection(&(x)->cs); \
		(x)->inited = FALSE; \
	} while (0)
#define	mutex_enter(x) \
	do { \
		CPDLC_ASSERT((x)->inited); \
		EnterCriticalSection(&(x)->cs); \
	} while (0)
#define	mutex_exit(x) \
	do { \
		CPDLC_ASSERT((x)->inited); \
		LeaveCriticalSection(&(x)->cs); \
	} while (0)

static DWORD _cpdlc_thread_start_routine(void *arg) CPDLC_UNUSED_ATTR;
static DWORD
_cpdlc_thread_start_routine(void *arg)
{
	cpdlc_thread_info_t *ti = (cpdlc_thread_info_t *)arg;
	ti->proc(ti->arg);
	free(ti);
	return (0);
}

static inline bool
thread_create(thread_t *thrp, void (*proc)(void *), void *arg)
{
	cpdlc_thread_info_t *ti =
	    (cpdlc_thread_info_t *)safe_calloc(1, sizeof (*ti));
	ti->proc = proc;
	ti->arg = arg;
	if ((*(thrp) = CreateThread(NULL, 0, _cpdlc_thread_start_routine, ti,
	    0, NULL)) != NULL) {
		return (true);
	}
	free(ti);
	return (false);
}

#define	thread_join(thrp) \
	CPDLC_VERIFY3S(WaitForSingleObject(*(thrp), INFINITE), ==, \
	    WAIT_OBJECT_0)
#define	thread_set_name(name)		CPDLC_UNUSED(name)

static inline uint64_t
cpdlc_thread_microclock(void)
{
	LARGE_INTEGER val, freq;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&val);
	return (((double)val.QuadPart / (double)freq.QuadPart) * 1000000.0);
}

#define	cv_wait(cv, mtx) \
	CPDLC_VERIFY(SleepConditionVariableCS((cv), &(mtx)->cs, INFINITE))
static inline int
cv_timedwait(condvar_t *cv, mutex_t *mtx, uint64_t limit)
{
	uint64_t now = cpdlc_thread_microclock();
	if (now < limit) {
		/*
		 * The only way to guarantee that when we return due to a
		 * timeout the full microsecond-accurate quantum has elapsed
		 * is to round-up to the nearest millisecond.
		 */
		if (SleepConditionVariableCS(cv, &mtx->cs,
		    ceil((limit - now) / 1000.0)) != 0) {
			return (0);
		}
		if (GetLastError() == ERROR_TIMEOUT)
			return (ETIMEDOUT);
		return (-1);
	}
	return (ETIMEDOUT);
}
#define	cv_init		InitializeConditionVariable
#define	cv_destroy(cv)	/* no-op */
#define	cv_broadcast	WakeAllConditionVariable

#define	cpdlc_usleep(x)	SleepEx((x) / 1000, FALSE)

#define	CPDLC_VERIFY_MUTEX_HELD(mtx)		(void)1
#define	CPDLC_VERIFY_MUTEX_NOT_HELD(mtx)	(void)1

#endif	/* IBM */

#ifdef	DEBUG
#define	CPDLC_ASSERT_MUTEX_HELD(mtx)		CPDLC_VERIFY_MUTEX_HELD(mtx)
#define	CPDLC_ASSERT_MUTEX_NOT_HELD(mtx)	CPDLC_VERIFY_MUTEX_NOT_HELD(mtx)
#else	/* !DEBUG */
#define	CPDLC_ASSERT_MUTEX_HELD(mtx)
#define	CPDLC_ASSERT_MUTEX_NOT_HELD(mtx)
#endif	/* !DEBUG */

#ifdef __cplusplus
}
#endif

#endif	/* _LIBCPDLC_THREAD_H_ */

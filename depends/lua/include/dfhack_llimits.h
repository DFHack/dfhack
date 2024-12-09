/**
Copyright © 2018 Pauli <suokkos@gmail.com>

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
   not claim that you wrote the original software. If you use this
   software in a product, an acknowledgment in the product
   documentation would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
   must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
   distribution.
 */

#pragma once

#ifdef _MSC_VER
#include <windows.h>
#else
#include <pthread.h>
#endif

#include <stdlib.h>

/*! \file dfhack_llimits.h
 * dfhack specific lua porting header that overrides lua defaults for thread
 * safety.
 */

#ifdef _MSC_VER
typedef CRITICAL_SECTION mutex_t;
#else
typedef pthread_mutex_t mutex_t;
#endif

struct lua_extra_state {
    mutex_t* mutex;
};

#define luai_mutex(L) ((lua_extra_state*)lua_getextraspace(L))->mutex

#ifdef _MSC_VER
#define luai_userstateopen(L) luai_mutex(L) = (mutex_t*)malloc(sizeof(mutex_t)); InitializeCriticalSection(luai_mutex(L))
#define luai_userstateclose(L) lua_unlock(L); DeleteCriticalSection(luai_mutex(L)); free(luai_mutex(L))
#define lua_lock(L) EnterCriticalSection(luai_mutex(L))
#define lua_unlock(L) LeaveCriticalSection(luai_mutex(L))
#else
#define luai_userstateopen(L) do { \
      luai_mutex(L) = (mutex_t*)malloc(sizeof(mutex_t)); \
      pthread_mutexattr_t attr; \
      pthread_mutexattr_init(&attr); \
      pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE); \
      pthread_mutex_init(luai_mutex(L), &attr); \
   } while (0)
#define luai_userstateclose(L) do { \
      lua_unlock(L); \
      pthread_mutex_destroy(luai_mutex(L)); \
      free(luai_mutex(L)); \
   } while (0)
#define lua_lock(L) pthread_mutex_lock(luai_mutex(L))
#define lua_unlock(L) pthread_mutex_unlock(luai_mutex(L))
#endif

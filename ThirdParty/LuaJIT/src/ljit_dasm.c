/*
** Wrapper for architecture-specific DynASM encoder.
** Copyright (C) 2005-2012 Mike Pall. See Copyright Notice in luajit.h
*/

#define ljit_dasm_c
#define LUA_CORE


#include "lua.h"

#include "ljit.h"
#include "ljit_dasm.h"
#include "lmem.h"


/* Glue macros for DynASM memory allocation. */
#define DASM_M_GROW(J, t, p, sz, need) \
  do { \
    size_t _sz = (sz), _need = (need); \
    if (_sz < _need) { \
      if (_sz < 16) _sz = 16; \
      while (_sz < _need) _sz += _sz; \
      (p) = (t *)luaM_realloc_(J->L, (p), (sz), _sz); \
      (sz) = _sz; \
    } \
  } while(0)

#define DASM_M_FREE(J, p, sz)	luaM_freemem(J->L, p, sz)

/* Embed architecture-specific DynASM encoder. */
#if defined(__i386) || defined(__i386__) || defined(_M_IX86)
#include "dasm_x86.h"

#elif defined(_M_X64) || defined(__x86_64__)

/*
** No native DynASM encoder exists (yet) for x64. These stubs only need to
** link -- they are never called because luaJIT_initbackend() (in
** ljit_backend.c) always fails on x64, keeping the JIT engine disabled
** and LuaJIT running purely as a bytecode interpreter.
*/

void dasm_init(Dst_DECL, int maxsection)
{
  (void)Dst_REF; (void)maxsection;
}

void dasm_free(Dst_DECL)
{
  (void)Dst_REF;
}

void dasm_setupglobal(Dst_DECL, void **gl, unsigned int maxgl)
{
  (void)Dst_REF; (void)gl; (void)maxgl;
}

void dasm_growpc(Dst_DECL, unsigned int maxpc)
{
  (void)Dst_REF; (void)maxpc;
}

void dasm_setup(Dst_DECL, dasm_ActList actionlist)
{
  (void)Dst_REF; (void)actionlist;
}

void dasm_put(Dst_DECL, int start, ...)
{
  (void)Dst_REF; (void)start;
}

int dasm_link(Dst_DECL, size_t *szp)
{
  (void)Dst_REF;
  if (szp) *szp = 0;
  return 1;  /* Non-zero: report an error if ever reached. */
}

int dasm_encode(Dst_DECL, void *buffer)
{
  (void)Dst_REF; (void)buffer;
  return 1;  /* Non-zero: report an error if ever reached. */
}

int dasm_getpclabel(Dst_DECL, unsigned int pc)
{
  (void)Dst_REF; (void)pc;
  return -1;
}

#else
#error "No support for this architecture (yet)"
#endif



/*
** LuaJIT VM builder.
** Copyright (C) 2005-2017 Mike Pall. See Copyright Notice in luajit.h
*/

#ifndef _BUILDVM_H
#define _BUILDVM_H

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ravi_def.h"
#include "ravi_arch.h"

/* Hardcoded limits. Increase as needed. */
#define BUILD_MAX_RELOC		200	/* Max. number of relocations. */

/* Prefixes for generated labels. */
#define LABEL_PREFIX		"ravi_"
#define LABEL_PREFIX_BC		LABEL_PREFIX "BC_"
#define LABEL_PREFIX_FF		LABEL_PREFIX "ff_"
#define LABEL_PREFIX_CF		LABEL_PREFIX "cf_"
#define LABEL_PREFIX_FFH	LABEL_PREFIX "ffh_"
#define LABEL_PREFIX_LIBCF	LABEL_PREFIX "lib_cf_"
#define LABEL_PREFIX_LIBINIT	LABEL_PREFIX "lib_init_"

/* Forward declaration. */
struct dasm_State;

/* Build modes. */
#define BUILDDEF(_) \
  _(elfasm) _(coffasm) _(machasm) _(peobj) _(raw) \
  _(bcdef) 

typedef enum {
#define BUILDENUM(name)		BUILD_##name,
BUILDDEF(BUILDENUM)
#undef BUILDENUM
  BUILD__MAX
} BuildMode;

/* Code relocation. */
typedef struct BuildReloc {
  int32_t ofs;
  int sym;
  int type;
} BuildReloc;

typedef struct BuildSym {
  const char *name;
  int32_t ofs;
} BuildSym;

/* Build context structure. */
typedef struct BuildCtx {
  /* DynASM state pointer. Should be first member. */
  struct dasm_State *D;
  /* Parsed command line. */
  BuildMode mode;
  FILE *fp;
  const char *outname;
  char **args;
  /* Code and symbols generated by DynASM. */
  uint8_t *code;
  size_t codesz;
  int npc, nglob, nsym, nreloc, nrelocsym;
  void **glob;
  BuildSym *sym;
  const char **relocsym;
  int32_t *bc_ofs;
  const char *beginsym;
  /* Strings generated by DynASM. */
  const char *const *globnames;
  const char *const *extnames;
  const char *dasm_ident;
  const char *dasm_arch;
  /* Relocations. */
  BuildReloc reloc[BUILD_MAX_RELOC];
} BuildCtx;

extern void owrite(BuildCtx *ctx, const void *ptr, size_t sz);
extern void emit_asm(BuildCtx *ctx);
extern void emit_peobj(BuildCtx *ctx);

extern const char *const bc_names[];

#endif
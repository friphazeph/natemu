// cut.h: Simple C Utilities single-header library
//
// Copyright (C) 2026 Maxime Delhaye
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

#ifndef CUT_H_
#define CUT_H_

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdbit.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

/* ===== Macro tricks ===== */

#define UNUSED(x) (void)(x)
#define decay(x) (0 ? (x) : (x))
#define typeof_decay(x) typeof(decay(x))
#define STACK_REF(x) &(((struct {typeof_decay(x) _;}) {._ = x})._)
#define of_hdr(h) ((void *)((h) + 1))
#define hdr_of(T_hdr, p) ((T_hdr *)(p) - 1)
#define align_size(x) (((x) + _Alignof(max_align_t) - 1) & ~(_Alignof(max_align_t) - 1))
#define ALIGNED_STRUCT(...) \
	union {                            \
		max_align_t _align_do_not_use; \
		struct __VA_ARGS__;            \
	}
#define TODO(message) do {                                             \
	fprintf(stderr, "%s:%d: TODO: %s\n", __FILE__, __LINE__, message); \
	abort();                                                           \
} while (0)

/* ===== Memory ===== */

typedef struct allocator Allocator;
typedef void *malloc_t(Allocator *, size_t);
typedef void *realloc_t(Allocator *, size_t, void *, size_t);
typedef void free_t(Allocator *, void *);
typedef void free_all_t(Allocator *);
typedef struct allocator {
	malloc_t   *malloc;
	realloc_t  *realloc;
	free_t     *free;
	free_all_t *free_all;
} Allocator;

static inline void *alloc_malloc(Allocator *a, size_t n) {
	if(!a) return malloc(n);
	return a->malloc(a, n);
}
static inline void *alloc_realloc(Allocator *a, size_t o, void *p, size_t n) {
	if(!a) return realloc(p, n);
	return a->realloc(a, o, p, n);
}
static inline void alloc_free(Allocator *a, void *p) {
	if(!a) {free(p); return;}
	a->free(a, p);
}
static inline void alloc_free_all(Allocator *a) {
	if(!a) fprintf(stderr, "FATAL: default malloc has no `free_all`\n"), exit(1);
	a->free_all(a);
}

#define oom_die() fprintf(stderr, "FATAL: ran out of memory ! Exiting.\n"), exit(1)

static inline void *malloc_or_die(Allocator *a, size_t sz) {
	void *p = alloc_malloc(a, sz);
	if (!p) oom_die();
	return p;
}

static inline void *realloc_or_die(Allocator *a, size_t sz_old, void *p_old, size_t sz) {
	void *p = alloc_realloc(a, sz_old, p_old, sz);
	if (!p) oom_die();
	return p;
}

/* - Arena alloc - */
Allocator *arena_new();
size_t     arena_save(Allocator *a);
void       arena_restore(Allocator *a, size_t marker);

#define scratch_region(arena) \
	for (size_t cut__s = arena_save(arena), cut__once_ = 0; !cut__once_; cut__once_ = 1, arena_restore(arena, cut__s))

/* - Pool alloc - */
typedef struct {Allocator *alloc; bool extend;} cut__pool_new_args;
Allocator *cut__pool_new(size_t n, size_t block_sz, cut__pool_new_args args);

#define pool_new(n, block_sz, ...) cut__pool_new(n, block_sz, (cut__pool_new_args) {.alloc = NULL, .extend = false, __VA_ARGS__})

/* - Internal temp alloc - */
#define temp_ctx \
	for (size_t cut__s = (cut__temp_arena ? arena_save(cut__temp_arena) : 0), cut__once_ = 0; \
			!cut__once_; \
			cut__once_ = 1, arena_restore(arena, cut__s) )

/* - Files - */
typedef struct {size_t cur, total; uint8_t *bytes, *base;} File;
typedef struct {Allocator *alloc;} cut__file_byte_arr_args;

File mmap_file(const char *name);
void munmap_file(File f);
uint8_t *cut__file_byte_arr(File *f, size_t n, cut__file_byte_arr_args args);

#define file_byte_arr(f, n, ...) \
	cut__file_byte_arr(f, n, (cut__file_byte_arr_args) {.alloc = NULL, __VA_ARGS__})
void *file_advance(File *f, size_t n);

/* ===== Dynamic Arrays ===== */

typedef ALIGNED_STRUCT ({
	size_t len;
	size_t cap;
	Allocator *alloc;
}) arr_hdr;

/* Internals */
typedef struct {Allocator *alloc; size_t init_cap; bool zero;} cut__arr_new_args;
void *cut__arr_new(size_t it_sz, cut__arr_new_args a);
void *cut__arr_append(void *p, void *it, size_t it_sz);
void *cut__arr_pop(void *p, size_t it_sz);
void *cut__arr_reserve(void *p, size_t n, size_t it_sz);

/* API */
/* returns a reference */
#define arr_pop(p) ((typeof(p)) cut__arr_pop(p, sizeof(p[0])))
#define arr_last(p) ((p)[arr_len(p)-1])
#define arr_clear(p) (hdr_of(arr_hdr, p)->len = 0)
#define arr_new(T, ...) ((T *) cut__arr_new(sizeof(T), (cut__arr_new_args) {.init_cap = 0, __VA_ARGS__}))
/* /!\ arr_append has a chance to invalidate old pointers. 
 * /!\ arr_append evaluates p multiple times. 
 * arr_append works with literals and rvalues */
#define arr_append(p, it) ((p) = cut__arr_append(p, STACK_REF(it), sizeof(*(p))))
/* /!\ arr_reserve has a chance to invalidate old pointers. 
 * /!\ arr_reserve evaluates p multiple times. */
#define arr_reserve(p, n) ((p) = cut__arr_reserve(p, n, sizeof(*(p))))
#define arr_set_len(p, n) (hdr_of(arr_hdr, p)->len = (n > hdr_of(arr_hdr, p)->cap) ? hdr_of(arr_hdr, p)->cap : n)
#define arr_foreach(it, p) for (typeof(p) it = (p), cut__p_ = it; it && (it < (cut__p_)+hdr_of(arr_hdr, cut__p_)->len); it++)
void    arr_free(void *p);
size_t  arr_len(void *p);

/* ===== Strings ===== */

typedef char * String;

typedef struct {Allocator *alloc; size_t init_cap;} cut__string_new_args;
String cut__string_new(char *lit, size_t len, cut__string_new_args args);
String cut__string_append(char *s, char *lit, size_t len);

#define string_new(...) cut__string_new(NULL, 0, (cut__string_new_args) {__VA_ARGS__})
#define string_new_lit(lit, ...) cut__string_new(lit, sizeof(lit) - 1, (cut__string_new_args) {__VA_ARGS__})
#define string_reserve(s, n) arr_reserve(s, n)
#define string_len(s) arr_len(s)
#define string_append(s1, s2) ((s1) = cut__string_append(s1, s2, string_len(s2)))
#define string_append_lit(s1, s2) ((s1) = cut__string_append(s1, s2, sizeof(s2) - 1))
#define string_terminate(s) arr_append(s, '\0')

typedef struct {
	size_t start;
	size_t end;
	String source;
} StringView;

char *cut__sv_temp_cstr(StringView sv);

#define stringview(s) ((StringView) {.source = s, .end = string_len(s) - 1})
#define sv_chop_left(sv) do { \
	StringView *cut__sv_ = &(sv);                           \
	if (cut__sv_->start < cut__sv_->end) cut__sv_->start++; \
} while(0)
#define sv_chop_right(sv) do { \
	StringView *cut__sv_ = &(sv);                         \
	if (cut__sv_->start < cut__sv_->end) cut__sv_->end--; \
} while(0)
/* Put these in temp_ctx to make sure temp memory doesn't expand, if you care about that */
#define sv_temp_cstr(sv) cut__sv_temp_cstr(sv)
StringView *sv_split_char(StringView sv, char c);

/* ===== Rings ===== */

typedef struct {
	size_t head;
	size_t tail;
	size_t it_sz;
	size_t count;
	Allocator *alloc;
	unsigned char *data;
} Ring;

typedef struct {Allocator *alloc;} cut__ring_new_args;
Ring cut__ring_new(size_t count, size_t it_sz, cut__ring_new_args args);
void *cut__ring_push(Ring *r, void *it);
void *cut__ring_pop(Ring *r);

#define ring_new(T, count, ...) cut__ring_new(count, sizeof(T), (cut__ring_new_args) {.alloc = NULL, __VA_ARGS__})
#define ring_push(r, it) ((typeof(it) *) cut__ring_push(r, STACK_REF(it)))
#define ring_pop(T, r) (((T *) cut__ring_pop(r)))

/* ===== Hash Maps ===== */

#ifndef MAP_INIT_CAP
#	define MAP_INIT_CAP 256
#endif
#ifndef MAP_MAX_OCC
#	define MAP_MAX_OCC 0.7
#endif

typedef uint64_t key_hash_t (unsigned char *, size_t);
typedef int      key_cmp_t  (unsigned char *, unsigned char *, size_t);

typedef struct {
	size_t key_sz;
	size_t val_sz;
	size_t key_sz_unaligned;
	size_t val_sz_unaligned;
	size_t capacity;
	size_t count;
	unsigned char *keys;
	unsigned char *vals;
	/* ideal_rank is 1 + (key_hash() % data_cap). 0 indicates inoccupancy */
	size_t        *ideal_ranks;
	key_hash_t    *key_hash;
	key_cmp_t     *key_cmp;
	Allocator     *alloc;
} Map;

typedef struct {
	size_t     init_cap;
	key_hash_t *hash_fn;
	key_cmp_t  *cmp_fn;
	Allocator  *alloc;
} cut__map_new_args;

/* Internals */
extern key_hash_t cut__default_key_hash;
extern key_cmp_t  cut__default_key_cmp;

size_t  cut__map_next_occ_idx(Map *map, size_t i);
void *  cut__map_get(Map *map, void *k);
bool    cut__map_remove(Map *map, void *k);
void    cut__map_resize(Map *map);
Map     cut__map_new(size_t key_sz_u, size_t val_sz_u, cut__map_new_args args);
void    cut__map_insert(Map *map, void *k, void *v);

#define cut__map_key_at(map, i) (((i) < (map)->capacity) ? cut__map_key_at_unchecked(map, i) : NULL)
#define cut__map_val_at(map, i) (((i) < (map)->capacity) ? cut__map_val_at_unchecked(map, i) : NULL)
#define cut__map_key_at_unchecked(map, i) &((map)->keys[(i) * (map)->key_sz])
#define cut__map_val_at_unchecked(map, i) &((map)->vals[(i) * (map)->val_sz])
#define CUT__MAP_NEW_ARGS(...) (cut__map_new_args) { \
	.init_cap = MAP_INIT_CAP,                    \
	.hash_fn  = cut__default_key_hash,           \
	.cmp_fn   = cut__default_key_cmp,            \
	__VA_ARGS__                                  \
}

/* API */
void    map_free(Map *map);
#define map_new(T1, T2, ...)      \
	cut__map_new(sizeof(T1), sizeof(T2), CUT__MAP_NEW_ARGS(__VA_ARGS__))
#define map_get(map, k, T) ((T *) \
	cut__map_get(map, STACK_REF(k)))
#define map_contains(map, k) \
	(cut__map_get(map, STACK_REF(k)) ? true : false)
/* /!\ Both map_remove and map_insert have a chance to invalidate old pointers from map_get */
#define map_remove(map, k)        \
	cut__map_remove(map, STACK_REF(k))
/* /!\ Both map_remove and map_insert have a chance to invalidate old pointers from map_get 
 * map_insert works with literals and rvalues, but by design, all static arrays will get decayed
 * and only their pointer stored, including string literals.
 * If you want to store fixed-size arrays, wrap them in a struct. */
#define map_insert(map, k, v)     \
	cut__map_insert(map, STACK_REF(k), STACK_REF(v))
#define map_foreach(Tk, Tv, it, map) \
	for ( \
			size_t cut__i_ = cut__map_next_occ_idx(map, 0), cut__brk_ = cut__i_ + 1;                     \
			(cut__i_ < (map)->capacity);                                                                 \
			cut__i_ = (cut__brk_ != cut__i_) ? (map)->capacity : cut__map_next_occ_idx(map, cut__i_ + 1) \
			) for ( \
				struct {Tk key; Tv val;} it = (typeof(it)) { \
				.key = (Tk) cut__map_key_at(map, cut__i_),   \
				.val = (Tv) cut__map_val_at(map, cut__i_)    \
				}; \
				cut__brk_ != cut__i_; \
				cut__brk_ = cut__i_)

typedef Map Set;

#define set_new(T, ...) \
	cut__map_new(sizeof(T), 0, CUT__MAP_NEW_ARGS(__VA_ARGS__))
#define set_insert(set, k) map_insert(set, k, 0)
#define set_contains(set, k) map_contains(set, k)
#define set_remove(set, k) map_remove(set, k)
#define set_foreach(Tk, it, set) \
	for ( \
			size_t cut__i_ = cut__map_next_occ_idx(set, 0), cut__brk_ = cut__i_ + 1;                     \
			(cut__i_ < (set)->capacity);                                                                 \
			cut__i_ = (cut__brk_ != cut__i_) ? (set)->capacity : cut__map_next_occ_idx(set, cut__i_ + 1) \
			) for ( Tk it = (Tk) cut__map_key_at(set, cut__i_); cut__brk_ != cut__i_; cut__brk_ = cut__i_)


/* - IMPLEMENTATION - */
#ifdef CUT_IMPLEMENTATION
/* ------ START ----- */

/* ===== Memory ===== */

/* - Arena alloc - */

#ifndef ARENA_CAP
#	define ARENA_CAP ((size_t)1 << 31)
#endif

typedef ALIGNED_STRUCT ({
	unsigned char *block;
	size_t capacity;
	size_t offset;
}) arena_hdr;

void *arena_malloc(Allocator *a, size_t n) {
	n = align_size(n);
	arena_hdr *h = hdr_of(arena_hdr, a);
	if (h->offset + n > h->capacity) return NULL;
	void *p = h->block + h->offset;
	h->offset += n;
	return p;
}

void *arena_realloc(Allocator *a, size_t old_sz, void *old_p, size_t sz) {
	old_sz = align_size(old_sz), sz = align_size(sz);
	if (old_sz >= sz) return old_p;
	void *p = arena_malloc(a, sz);
	if (!p) return NULL;
	if (!old_p) return p;

	memcpy(p, old_p, old_sz);
	return p;
}

// individual free's shouldn't do anything with arenas
void arena_free(Allocator *a, void *p) {UNUSED(a), UNUSED(p); return;}

void arena_free_all(Allocator *a) {
	arena_hdr *h = hdr_of(arena_hdr, a);
	munmap(h->block, h->capacity);
	free(h);
}

size_t arena_save(Allocator *a) {
	return a ? hdr_of(arena_hdr, a)->offset : 0;
}

void arena_restore(Allocator *a, size_t marker) {
	if (marker > hdr_of(arena_hdr, a)->offset) {
		fprintf(stderr, "ERROR: Tried to restore an arena forwards. Are you sure you used arena_save ?\n");
		return;
	}
	hdr_of(arena_hdr, a)->offset = marker;
}

Allocator *arena_new() {
	arena_hdr *h = malloc(sizeof(h[0]) + sizeof(Allocator));
	*h = (arena_hdr) {
		.capacity = ARENA_CAP,
		.block    = mmap(
				NULL,
				ARENA_CAP,
				PROT_READ | PROT_WRITE,
				MAP_ANONYMOUS | MAP_PRIVATE, -1, 0),
	};
	if (h->block == MAP_FAILED) oom_die();
	Allocator *a = of_hdr(h);
	*a = (Allocator) {
		.malloc   = arena_malloc,
		.free     = arena_free,
		.free_all = arena_free_all,
		.realloc  = arena_realloc
	};
	return a;
}

/* - Pool alloc - */

typedef ALIGNED_STRUCT ({
	size_t block_sz;
	size_t pool_sz;
	void **pools;
	void  *free_head;
	Allocator *alloc;
	bool extend;
}) pool_hdr;

void *pool_malloc(Allocator *a, size_t n) {
	pool_hdr *h = hdr_of(pool_hdr, a);
	if (n > h->block_sz) {
		fprintf(stderr, "FATAL: Tried to allocate %zu bytes in a Pool of block size %zu.\n", n, h->block_sz);
		exit(1);
	}
	void *ret = h->free_head;
	if (!ret) {
		if (!h->extend) return NULL;
		h->free_head = alloc_malloc(h->alloc, h->block_sz * h->pool_sz);
		arr_append(h->pools, h->free_head);

		for (size_t i = 0; i < h->pool_sz; i++) {
			void *cur  = (unsigned char *) h->free_head + i * h->block_sz;
			void *next = (unsigned char *) cur + h->block_sz;
			if (i == h->pool_sz - 1) next = NULL;
			memcpy(cur, &next, sizeof(void *));
		}

		ret = h->free_head;
	}
	memcpy(&h->free_head, ret, sizeof(void *));
	return ret;
}

void *pool_realloc(Allocator *a, size_t old_sz, void *old_p, size_t sz) {
	pool_hdr *h = hdr_of(pool_hdr, a);
	if (sz > h->block_sz) {
		fprintf(stderr, "FATAL: Tried to allocate %zu bytes in a Pool of block size %zu.\n", sz, h->block_sz);
		exit(1);
	}
	UNUSED(a), UNUSED(old_sz);
	return old_p;
}

void pool_free(Allocator *a, void *p) {
	pool_hdr *h = hdr_of(pool_hdr, a);
	void *temp = h->free_head;
	memcpy(&h->free_head, &p, sizeof(void *));
	memcpy(p, &temp, sizeof(void *));
}

void pool_free_all(Allocator *a) {
	pool_hdr *h = hdr_of(pool_hdr, a);
	arr_foreach(it, h->pools) {
		alloc_free(h->alloc, *it);
	}
	arr_free(h->pools);
	alloc_free(h->alloc, h);
}

Allocator *cut__pool_new(size_t n, size_t block_sz, cut__pool_new_args args) {
	n = stdc_bit_ceil(n);
	if (block_sz < sizeof(void *))
		block_sz = sizeof(void *);

	pool_hdr *h = alloc_malloc(args.alloc, sizeof(h[0]) + sizeof(Allocator));
	*h = (pool_hdr) {
		.block_sz = block_sz,
		.pool_sz  = n,
		.pools    = arr_new(void *, .alloc = args.alloc),
		.alloc    = args.alloc,
		.extend   = args.extend
	};
	arr_append(h->pools, alloc_malloc(args.alloc, block_sz * n));
	h->free_head = h->pools[0];

	for (size_t i = 0; i < n; i++) {
		void *cur  = (unsigned char *) h->free_head + i * block_sz;
		void *next = (unsigned char *) cur + block_sz;
		if (i == n - 1) next = NULL;
		memcpy(cur, &next, sizeof(void *));
	}

	Allocator *a = of_hdr(h);
	*a = (Allocator) {
		.malloc   = pool_malloc,
		.realloc  = pool_realloc,
		.free     = pool_free,
		.free_all = pool_free_all
	};
	return a;
}

/* - Internal temp alloc - */

_Thread_local static Allocator *cut__temp_arena = NULL;

/* - Files - */

File mmap_file(const char *name) {
    int fd = open(name, O_RDONLY);
    if (fd == -1) {
		perror("Problem opening file");
		return (File){0};
	}

    struct stat s;
    if (fstat(fd, &s) == -1) { close(fd); return (File){0}; }

    long page_size  = sysconf(_SC_PAGESIZE);
    size_t prefix   = (sizeof(arr_hdr) + page_size - 1) & ~(page_size - 1);
    size_t total    = prefix + s.st_size;

    uint8_t *base = mmap(NULL, total,
                         PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED) { close(fd); return (File){0}; }

    uint8_t *bytes = mmap(base + prefix, s.st_size,
                          PROT_READ,
                          MAP_PRIVATE | MAP_FIXED, fd, 0);
    close(fd);
    if (bytes == MAP_FAILED) { munmap(base, total); return (File){0}; }

	*hdr_of(arr_hdr, bytes) = (arr_hdr) {
		.cap = s.st_size,
		.len = s.st_size,
	};

    return (File){ .total = total, .cur = 0, .bytes = bytes, .base = base };
}

void munmap_file(File f) {
	munmap(f.base, f.total);
}

void *file_advance(File *f, size_t n) {
	if (f->cur >= arr_len(f->bytes)) return NULL;
	void *ret = f->bytes + f->cur;
	f->cur += n;
	return ret;
}

uint8_t *cut__file_byte_arr(File *f, size_t n, cut__file_byte_arr_args args) {
	n = (n > arr_len(f->bytes) - f->cur) ? arr_len(f->bytes) - f->cur : n;
	uint8_t *arr = arr_new(uint8_t, .alloc = args.alloc, .init_cap = n);
	hdr_of(arr_hdr, arr)->len = n;
	memcpy(arr, f->bytes + f->cur, n);
	f->cur += n;
	return arr;
}

/* ===== Dynamic Arrays ===== */

#ifndef ARR_INIT_CAP
#	define ARR_INIT_CAP 256
#endif

void *cut__arr_new(size_t it_sz, cut__arr_new_args a) {
	size_t init_cap = stdc_bit_ceil(a.init_cap ? a.init_cap : ARR_INIT_CAP);
	arr_hdr *h = malloc_or_die(a.alloc, sizeof(h[0]) + it_sz * init_cap);
	*h = (arr_hdr) {
		.len = 0,
		.cap = init_cap,
		.alloc = a.alloc,
	};
	if (a.zero) {
		memset(of_hdr(h), 0, init_cap);
	}
	return of_hdr(h);
}

void *cut__arr_append(void *p, void *it, size_t it_sz) {
	if (!(p)) p = cut__arr_new(it_sz, (cut__arr_new_args) {0});
	arr_hdr *h = hdr_of(arr_hdr, p);
	if (h->len >= h->cap) {
		p = cut__arr_reserve(p, h->cap*2, it_sz);
		h = hdr_of(arr_hdr, p);
	}
	void *addr = ((unsigned char *)p) + it_sz * h->len++;
	memcpy(addr, it, it_sz);
	return p;
}

void *cut__arr_pop(void *p, size_t it_sz) {
	if (!p || !arr_len(p)) return NULL;
	arr_hdr *h = hdr_of(arr_hdr, p);
	h->len--;
	return (unsigned char *) p + it_sz * h->len;
}

void *cut__arr_reserve(void *p, size_t n, size_t it_sz) {
	if (!(p)) { 
		p = cut__arr_new(it_sz, (cut__arr_new_args) {.init_cap = n});
		return p;
	}
	arr_hdr *h = hdr_of(arr_hdr, p);
	if (h->cap >= n) return p;
	size_t old_sz = sizeof(h[0]) + it_sz * h->cap;
	size_t new_cap = stdc_bit_ceil(n);
	h = realloc_or_die(h->alloc, old_sz, h, sizeof(h[0]) + new_cap * it_sz);
	h->cap = new_cap;
	p = of_hdr(h);
	return p;
}

void arr_free(void *p) {
	if (!p) return;
	arr_hdr *h = hdr_of(arr_hdr, p);
	alloc_free(h->alloc, h);
}

size_t arr_len(void *p) {
	return ((p) ? hdr_of(arr_hdr, p)->len : 0);
}

/* ===== Strings ===== */

String cut__string_new(char *lit, size_t len, cut__string_new_args args) {
	len = (len > args.init_cap) ? len : args.init_cap;
	String s = arr_new(char, .init_cap = len, .alloc = args.alloc);
	cut__string_append(s, lit, len);
	return s;
}

String cut__string_append(String s, char *lit, size_t len) {
	string_reserve(s, string_len(s) + len);
	memcpy(s + string_len(s), lit, len);
	hdr_of(arr_hdr, s)->len += len;
	return s;
}

char *cut__sv_temp_cstr(StringView sv) {
	if (!cut__temp_arena) cut__temp_arena = arena_new();
	char *ret = alloc_malloc(cut__temp_arena, (sv.end - sv.start) + 1);
	memcpy(ret, sv.source + sv.start, sv.end - sv.start);
	ret[sv.end-sv.start] = '\0';
	return ret;
}

StringView *sv_split_char(StringView sv, char c) {
	if (!cut__temp_arena) cut__temp_arena = arena_new();
	StringView *out = arr_new(StringView, .alloc=cut__temp_arena);
	for (size_t i = sv.start; i <= sv.end; i++) {
		StringView new = (StringView) {.start = i, .source = sv.source};
		while (i <= sv.end && sv.source[i] != c) i++;
		new.end = i;
		arr_append(out, new);
	}
	return out;
}

/* ===== Rings ===== */

Ring cut__ring_new(size_t count, size_t it_sz, cut__ring_new_args args) {
	Ring ret = (Ring) {
		.it_sz = it_sz,
		.count = count + 1,
		.alloc = args.alloc,
		.data  = alloc_malloc(args.alloc, (count + 1) * it_sz),
	};
	return ret;
}

void *cut__ring_push(Ring *r, void *it) {
	if ((r->head + 1) % r->count == r->tail) return NULL;
	void *ret = memcpy(r->data + r->head*r->it_sz, it, r->it_sz);
	r->head = (r->head + 1) % r->count;
	return ret;
}

void *cut__ring_pop(Ring *r) {
	if (r->head == r->tail) return NULL;
	void *ret = r->data + r->tail*r->it_sz;
	r->tail = (r->tail + 1) %r->count;
	return ret;
}

/* ===== Hash Maps ===== */

#define MAP_OCCUPIED(map, i) ((map)->ideal_ranks[i] != 0)
#define MAP_IDEAL(map, i) ((map)->ideal_ranks[i] == ((i) + 1))

void map_free(Map *map) {
	alloc_free(map->alloc, map->keys);
	alloc_free(map->alloc, map->vals);
	alloc_free(map->alloc, map->ideal_ranks);
	*map = (Map){0};
}

uint64_t cut__default_key_hash(unsigned char *key, size_t key_sz) {
	uint64_t h = 1469598103934665603ULL;
	for (size_t i = 0; i < key_sz; i++) {
		h ^= (unsigned int) key[i];
		h *= 1099511628211ULL;
	}
	return h;
}

int cut__default_key_cmp(unsigned char *key1, unsigned char *key2, size_t key_sz) {
	for (size_t i = 0; i < key_sz; i++) {
		int diff = key1[i] - key2[i];
		if (diff != 0) return diff;
	}
	return 0;
}

Map cut__map_new(size_t key_sz_u, size_t val_sz_u, cut__map_new_args args) {
	size_t key_sz_a       = align_size(key_sz_u);
	size_t val_sz_a       = align_size(val_sz_u);
	size_t *ideal_ranks = malloc_or_die(args.alloc, sizeof(ideal_ranks[0]) * args.init_cap);
	memset(ideal_ranks, 0, sizeof(ideal_ranks[0]) * args.init_cap);

	return (Map) {
		.key_sz           = key_sz_a,
		.val_sz           = val_sz_a,
		.key_sz_unaligned = key_sz_u,
		.val_sz_unaligned = val_sz_u,
		.capacity         = args.init_cap,
		.keys             = malloc_or_die(args.alloc, key_sz_a * args.init_cap),
		.vals             = malloc_or_die(args.alloc, val_sz_a * args.init_cap),
		.ideal_ranks      = ideal_ranks,
		.key_hash         = args.hash_fn,
		.key_cmp          = args.cmp_fn,
		.alloc            = args.alloc
	};
}

/* returns i >= cap on end */
size_t cut__map_next_occ_idx(Map *map, size_t i) {
	for (; i < map->capacity; i++)
		if (MAP_OCCUPIED(map, i))
			return i;
	return map->capacity;
}

void cut__map_insert_no_resize(Map *map, void *k, void *v) {
	uint64_t h = map->key_hash(k, map->key_sz_unaligned);
	size_t i = h % map->capacity;
	size_t init_i = i;

	for (size_t n = 0; n < map->capacity; n++) {
		if (!MAP_OCCUPIED(map, i)) {
			map->count++;
			break;
		}
		if (map->key_cmp(cut__map_key_at(map, i), k, map->key_sz_unaligned) == 0) break;
		i = (i+1) % map->capacity;
	}

	map->ideal_ranks[i] = init_i + 1;
	memcpy(cut__map_key_at_unchecked(map, i), k, map->key_sz_unaligned);
	memcpy(cut__map_val_at_unchecked(map, i), v, map->val_sz_unaligned);
}

void cut__map_resize(Map *map) {
	Map old = *map;
	
	map->capacity = map->capacity ? map->capacity * 2 : MAP_INIT_CAP;
	map->keys = malloc_or_die(map->alloc, map->key_sz * map->capacity);
	map->vals = malloc_or_die(map->alloc, map->val_sz * map->capacity);
	map->ideal_ranks = malloc_or_die(map->alloc, sizeof(map->ideal_ranks[0]) * map->capacity);
	memset(map->ideal_ranks, 0, sizeof(map->ideal_ranks[0]) * map->capacity);

	map->count = 0;
	for (size_t i = 0; i < old.capacity; i++) {
		if (!MAP_OCCUPIED(&old, i)) continue;
		cut__map_insert_no_resize(map,
						&(old.keys[i * (map)->key_sz]),
						&(old.vals[i * (map)->val_sz]));
	}
	map_free(&old);
}

void cut__map_insert(Map *map, void *k, void *v) {
	if (map->count > MAP_MAX_OCC * map->capacity) cut__map_resize(map);
    cut__map_insert_no_resize(map, k, v);
}

size_t cut__map_find_key_rank(Map *map, void *k) {
	uint64_t h = map->key_hash(k, map->key_sz_unaligned);
	size_t i = h % map->capacity;

	size_t n;
	size_t idx;
	for (n = 0; n < map->capacity; n++)  {
		idx = (i + n) % map->capacity;
		if (!MAP_OCCUPIED(map, idx)) return 0;
		if (map->key_cmp(cut__map_key_at(map, idx), k, map->key_sz_unaligned) == 0) break;
	}
	if (n == map->capacity) return 0;
	return idx + 1;
}

bool cut__map_remove(Map *map, void *k) {
	size_t hole = cut__map_find_key_rank(map, k);
	if (!hole) return false;
	hole--;
    size_t scan = (hole + 1) % map->capacity;
    for (size_t counted = 1; counted < map->capacity; counted++) {
        size_t ideal_rank = map->ideal_ranks[scan];
        if (ideal_rank == 0) break;

        size_t ideal_idx = ideal_rank - 1;
        size_t ideal_dist = (ideal_idx - hole + map->capacity) % map->capacity;
        size_t scan_dist  = (scan - hole + map->capacity) % map->capacity;

        if (ideal_dist && ideal_dist <= scan_dist) {
            scan = (scan + 1) % map->capacity;
            continue;
        }

        memcpy(cut__map_key_at_unchecked(map, hole), cut__map_key_at_unchecked(map, scan), map->key_sz_unaligned);
        memcpy(cut__map_val_at_unchecked(map, hole), cut__map_val_at_unchecked(map, scan), map->val_sz_unaligned);
        map->ideal_ranks[hole] = ideal_rank;

        hole = scan;
        scan = (scan + 1) % map->capacity;
    }

    map->ideal_ranks[hole] = 0;
    map->count--;

    return true;
}

void *cut__map_get(Map *map, void *k) {
	size_t i = cut__map_find_key_rank(map, k);
	if (!i) return NULL;
	return cut__map_val_at_unchecked(map, i - 1);
}

#endif /* CUT_IMPLEMENTATION */

#endif /* CUT_H_ */

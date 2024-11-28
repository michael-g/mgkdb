/*
Copyright 2024 Michael Guyver

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the “Software”), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/
#include <stdlib.h>      // NULL, abort, exit
#include <string.h>      // strlen, strerror, strncmp
#include <sys/stat.h>    // stat, shm_open constants
#include <stdint.h>      // ..
#include <stdio.h>       // printf
#include <unistd.h>      // close, getpid
#include <fcntl.h>       // open, O_* constants
#include <sys/mman.h>    // shm_open, mmap
#include <time.h>        // clock_gettime
#include <sys/time.h>
#include <errno.h>

#define KXVER 3
#include "k.h"

#define SZ_I8    1
#define SZ_I16   2
#define SZ_I32   4
#define SZ_I64   8
#define SZ_UUID 16

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define _MG_TRACE_ 0
#define _MG_DEBUG_ 1
#define _MG_INFO_  2
#define _MG_WARN_  3
#define _MG_ERROR_ 4

#ifndef _MG_LOG_LVL_
#define _MG_LOG_LVL_ _MG_INFO_
#endif

#define MG_LOG_TRA_ENABLED (_MG_LOG_LVL_ <= _MG_TRACE_)
#define MG_LOG_DBG_ENABLED (_MG_LOG_LVL_ <= _MG_DEBUG_)
#define MG_LOG_INF_ENABLED (_MG_LOG_LVL_ <= _MG_INFO_)
#define MG_LOG_WRN_ENABLED (_MG_LOG_LVL_ <= _MG_WARN_)
#define MG_LOG_ERR_ENABLED (_MG_LOG_LVL_ <= _MG_ERROR_)

/* see https://www.linuxjournal.com/article/8603 for other attributes like 'bright', 'dim' */
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RST   "\x1B[0m"

#define LOG(file,lvl,fmt,...)      { fprintf(file, lvl ": " fmt " [" MAG "%s" RST "] " BLU  __FILE__ RST ":" CYN TOSTRING(__LINE__) "\n" RST,##__VA_ARGS__, __func__ ); fflush(file); }
#define TRA_PRINT(fmt, ...)        do { if (MG_LOG_TRA_ENABLED) LOG(stdout, BLU "TRACE" RST, fmt RST,##__VA_ARGS__)} while (0)
#define DBG_PRINT(fmt, ...)        do { if (MG_LOG_DBG_ENABLED) LOG(stdout, MAG "DEBUG" RST, fmt RST,##__VA_ARGS__)} while (0)
#define INF_PRINT(fmt, ...)        do { if (MG_LOG_INF_ENABLED) LOG(stdout, CYN " INFO" RST, fmt RST,##__VA_ARGS__)} while (0)
#define WRN_PRINT(fmt, ...)        do { if (MG_LOG_WRN_ENABLED) LOG(stdout, YEL " WARN" RST, fmt RST,##__VA_ARGS__)} while (0)
#define ERR_PRINT(fmt, ...)        do { if (MG_LOG_ERR_ENABLED) LOG(stdout, RED "ERROR" RST, fmt RST,##__VA_ARGS__)} while (0)
#define ERR_EXIT(fmt, ...) do { ERR_PRINT(fmt,##__VA_ARGS__); exit(EXIT_FAILURE); } while (0)


inline static uint64_t mg_gettime(void)
{
  struct timespec t;
  if (-1 == clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t)) {
    ERR_EXIT("clock_gettime: %s", strerror(errno));
  }
  return t.tv_nsec + t.tv_sec * 1000 * 1000 * 1000;
}

//----------------------------------------------------------------------------------------------------------------
static uint64_t mg_read_uuid(const int8_t *src, U *dst)
{
  memcpy(dst->g, src, SZ_UUID);
  return SZ_UUID;
}
static uint64_t mg_read_i8(const int8_t *src, uint8_t *dst)
{
  memcpy(dst, (void*)src, sizeof(*dst));
  return SZ_I8;
}
static int64_t mg_read_i16(const int8_t *src, int16_t *dst)
{
  memcpy(dst, (void*)src, sizeof(*dst));
  return SZ_I16;
}
static uint64_t mg_read_i32(const int8_t *src, int32_t *dst)
{
  memcpy(dst, (void*)src, sizeof(*dst));
  return SZ_I32;
}
static uint64_t mg_read_i64(const int8_t *src, int64_t *dst)
{
  memcpy(dst, (void*)src, sizeof(*dst));
  return SZ_I64;
}

static uint64_t mg_read_i8_atom(const int8_t typ, const int8_t *src, K*ptr)
{
  *ptr = ka(typ);
  return mg_read_i8(src, &(*ptr)->g);
}
static uint64_t mg_read_i16_atom(const int8_t typ, const int8_t *src, K *ptr)
{
  *ptr = ka(typ);
  return mg_read_i16(src, &(*ptr)->h);
}
static uint64_t mg_read_i32_atom(const int8_t typ, const int8_t *src, K *ptr)
{
  *ptr = ka(typ);
  return mg_read_i32(src, &(*ptr)->i);
}
static uint64_t mg_read_i64_atom(const int8_t typ, const int8_t *src, K *ptr)
{
  *ptr = ka(typ);
  return mg_read_i64(src, (int64_t*)(&(*ptr)->j));
}
static uint64_t mg_read_uuid_atom(const int8_t typ, const int8_t *src, K *ptr)
{
  U g = {{0}};
  uint64_t len = mg_read_uuid(src, &g);
  *ptr = ku(g);
  return len;
}

static uint64_t mg_read_vec(const int8_t typ, const int8_t *src, K *ptr)
{
  uint64_t off = 0;
  int8_t att = src[off++];
  int32_t len;
  off += mg_read_i32(src + off, &len);
  K vec = ktn(typ, (J)len);
  vec->a = att;
  vec->n = len;
  TRA_PRINT("have vec.length of %i", len);
  switch (typ) {
    case KT: 
    case KV:
    case KU:
    case KD: 
    case KM:
    case KE:
    case KI: {
      memcpy(&kG(vec)[0], src + off, len * SZ_I32);
      off += len * SZ_I32;
      break;
    }
    case KN:
    case KZ:
    case KP:
    case KF:
    case KJ: {
      memcpy(&kG(vec)[0], src + off, len * SZ_I64);
      off += len * SZ_I64;
      break;
    }
    case KH: {
      memcpy(&kG(vec)[0], src + off, len * SZ_I16);
      off += len * SZ_I16;
      break;
    }
    case KC:
    case KG:
    case KB: {
      memcpy(&kG(vec)[0], src + off, len * SZ_I8);
      off += len * SZ_I8;
      break;
    }
    case UU: {
      memcpy(&kG(vec)[0], src + off, len * SZ_UUID);
      off += len * SZ_UUID;
      break;
    }
    case KS: {
      for (int32_t i = 0 ; i < len ; i++) {
        kS(vec)[i] = ss((char*)(src + off));
        size_t n = strlen((char*)(src + off));
        off += n + SZ_I8;
      }
      break;
    }
  }
  *ptr = vec;
  return off;
}

static uint64_t mg_read_vec_OLD(const int8_t typ, const int8_t *src)
{
  uint64_t off = 0;
  int8_t att = src[off++];
  int32_t len;
  off += mg_read_i32(src + off, &len);
  K vec = ktn(typ, (J)len);
  vec->a = att;
  vec->n = len;
  TRA_PRINT("have vec.length of %i", len);
  switch (typ) {
    case KT: 
    case KV:
    case KU:
    case KD: 
    case KM:
    case KE:
    case KI: {
      memcpy(&kG(vec)[0], src + off, len * SZ_I32);
      off += len * SZ_I32;
      break;
    }
    case KN:
    case KZ:
    case KP:
    case KF:
    case KJ: {
      memcpy(&kG(vec)[0], src + off, len * SZ_I64);
      off += len * SZ_I64;
      break;
    }
    case KH: {
      memcpy(&kG(vec)[0], src + off, len * SZ_I16);
      off += len * SZ_I16;
      break;
    }
    case KC:
    case KG:
    case KB: {
      memcpy(&kG(vec)[0], src + off, len * SZ_I8);
      off += len * SZ_I8;
      break;
    }
    case UU: {
      memcpy(&kG(vec)[0], src + off, len * SZ_UUID);
      off += len * SZ_UUID;
      break;
    }
    case KS: {
      for (int32_t i = 0 ; i < len ; i++) {
        kS(vec)[i] = ss((char*)(src + off));
        size_t n = strlen((char*)(src + off));
        off += n + SZ_I8;
      }
      break;
    }
  }
  return off;
}

static uint64_t mg_chk_match_and_get_len(const int8_t *const src, K tbl_names, int *relevant, int32_t level, int32_t index)
{
  level += 1;

  uint64_t off = 0;
  int8_t typ = src[off++];
  TRA_PRINT("have type %hhi", typ);

  switch (typ) {
    case -KT: 
    case -KV:
    case -KU:
    case -KD: 
    case -KM:
    case -KE:
    case -KI: {
      off += SZ_I32;
      return off;
    }
    case -KN:
    case -KZ:
    case -KP:
    case -KF:
    case -KJ: {
      off += SZ_I64;
      return off;
    }
    case -KH: {
      off += SZ_I16;
      return off;
    }
    case -KC:
    case -KG:
    case -KB: {
      off += SZ_I8;
      return off;
    }
    case -UU: {
      off += SZ_UUID;
      return off;
    }
    case -KS: {
      const char * pos = (char*)src + off;
      size_t len = (uint64_t)strlen(pos);
      TRA_PRINT("considering symbol atom %s with level %i and index %i", (const char*)(src + off), level, index);
      off += SZ_I8 + len;
      if (1 == level && 1 == index) {
        for (int64_t i = 0 ; i < tbl_names->n ; i++) {
          if (0 == strncmp(kS(tbl_names)[i], pos, len + SZ_I8)) {
            *relevant = 1;
          }
        }
      }
      return off;
    }
    case 0: {
      int8_t att = src[off++];
      int32_t len;
      off += mg_read_i32(src + off, &len);
      TRA_PRINT("have list.length %i", len);
      K lst = ktn(typ, (J)len);
      lst->a = att;
      lst->n = len;
      for (int32_t i = 0 ; i < len ; i++) {
        off += mg_chk_match_and_get_len(src + off, tbl_names, relevant, level, i);
      }
      return off;
    }
    case KB:
    case KC:
    case KG:
    case KH:
    case KI:
    case KE:
    case KM:
    case KD:
    case KU:
    case KV:
    case KT:
    case KJ:
    case KF:
    case KP:
    case KZ:
    case KN:
    case KS:
    {
      off += mg_read_vec_OLD(typ, src + off);
      return off;
    }
    case XT: {
      off += SZ_I8; // some sort of attribute byte
      off += mg_chk_match_and_get_len(src + off, tbl_names, relevant, level, 0);
      return off;
    }
    case XD: { // 99
      off += mg_chk_match_and_get_len(src + off, tbl_names, relevant, level, 0);
      off += mg_chk_match_and_get_len(src + off, tbl_names, relevant, level, 1);
      return off;
    }
    default: {
      ERR_PRINT("unsupported type %hhi, cannot continue", typ);
      abort();
    }
  }
  // unreachable
  return off;
}

static uint64_t mg_chk_msg(const int8_t *const src, K tbl_names, int *relevant)
{
  int32_t level = -1;
  int32_t index = 0;
  return mg_chk_match_and_get_len(src, tbl_names, relevant, level, index);
}

static uint64_t mg_reify_msg(const int8_t *src, K *obj)
{
  uint64_t off = 0;
  int8_t typ = src[off++];

  switch (typ) {
    case -KT: 
    case -KV:
    case -KU:
    case -KD: 
    case -KM:
    case -KE:
    case -KI: {
      return off + mg_read_i32_atom(typ, src + off, obj);
    }
    case -KN:
    case -KZ:
    case -KP:
    case -KF:
    case -KJ: {
      return off + mg_read_i64_atom(typ, src + off, obj);
    }
    case -KH: {
      return off + mg_read_i16_atom(typ, src + off, obj);
    }
    case -KC:
    case -KG:
    case -KB: {
      return off + mg_read_i8_atom(typ, src + off, obj);
    }
    case -UU: {
      return off + mg_read_uuid_atom(typ, src + off, obj);
    }
    case -KS: {
      char * pos = (char*)(src + off);
      const size_t len = (size_t)strlen(pos);
      TRA_PRINT("found symbol '%.*s'", (int)len, pos);
      *obj = ks(pos);
      return off + len + SZ_I8;
    }
    case 0: {
      int8_t att = src[off++];
      int32_t len;
      off += mg_read_i32(src + off, &len);
      TRA_PRINT("have list.length %i", len);
      K lst = *obj = ktn(typ, (J)len);
      lst->a = att;
      lst->n = len;
      for (int32_t i = 0 ; i < len ; i++) {
        K elem = NULL;
        TRA_PRINT("deserialising element[%i] of %i for list", i, len);
        off += mg_reify_msg(src + off, &elem);
        kK(lst)[i] = elem;
      }
      return off;
    }
    case KB:
    case KC:
    case KG:
    case KH:
    case KI:
    case KE:
    case KM:
    case KD:
    case KU:
    case KV:
    case KT:
    case KJ:
    case KF:
    case KP:
    case KZ:
    case KN:
    case KS:
    {
      return off + mg_read_vec(typ, src + off, obj);
    }
    case XT: {
      K dct;
      off += SZ_I8; // some sort of attribute byte
      off += mg_reify_msg(src + off, &dct);
      *obj = xT(dct);
      return off;
    }
    case XD: { // Dictionary 0x63 99d
      K kys, vls;
      off += mg_reify_msg(src + off, &kys);
      off += mg_reify_msg(src + off, &vls);
      *obj = xD(kys, vls);
      return off;
    }
    default: {
      ERR_PRINT("unsupported type %hhi, cannot continue", typ);
      abort();
    }
  }
}

static inline uint64_t mg_eval_msg(const int8_t *src)
{
  K msg = NULL;
  (void)mg_reify_msg(src, &msg);
  TRA_PRINT("evaluating message of length %lli", msg->n);
  k(0, "{value x}", msg, 0); // N.B. msg is unusable in this function after this call since we don't do r1(..)
  return 0;
}

static uint64_t mg_replay_msgs(const void *src, uint64_t off, const uint64_t size, const K tbl_names)
{
  uint64_t num_msgs = 0;
  do {
    int relevant = 0;

    uint64_t len = mg_chk_msg(src + off, tbl_names, &relevant);
    if (relevant) {
      TRA_PRINT("found relevant message at offset %zd with length %zd", off, len);
      num_msgs += 1;
      int err = mg_eval_msg(src + off);
      if (-1 == err) {
        exit(EXIT_FAILURE);
      }
    }
    else {
      TRA_PRINT("skipping message at offset %zd with length %zd", off, len);
    }
    off += len;
  } while (off < size);
  return num_msgs;
}

// 'path' to the journal as hsym, 'tbl_names' a sym-vec of table names to include
K mg_replay(K src_path, K tbl_names)
{
  // Require tbl_names to be a symbol vector
  if (KS != tbl_names->t)
    return krr("names.type");
  
  // Require src_path to be an hsym atom
  k(0, "{[P] $[not -11h~type P;'\"path.type:\",.Q.s1 P;not\":\"~first string P;'\"path.hsym:\",.Q.s1 P]}", r1(src_path), 0);

  struct stat buf = {0};
  int err = stat(&src_path->s[1], &buf);
  if (-1 == err)
    return krr("stat.fail");

  if (S_IFLNK != (S_IFMT & buf.st_mode) && S_IFREG != (S_IFMT & buf.st_mode))
    return krr("src_path.filetype");

  const uint64_t size = (uint64_t)buf.st_size; // defined as __off_t for me, C.f. struct_stat.h

  DBG_PRINT("File is %zd bytes in length", size);

  const int src_fd = open(&src_path->s[1], O_RDONLY, O_NOATIME);
  if (-1 == src_fd) {
    ERR_PRINT("failed to open %s: %s", src_path->s, strerror(errno));
    return krr("open.src");
  }
  DBG_PRINT("opened source-file %s as FD %i", src_path->s, src_fd);

  char *err_msg = NULL;
  char jnl_hdr_buf[sizeof(uint64_t)];

  ssize_t nrd = read(src_fd, jnl_hdr_buf, sizeof(jnl_hdr_buf));
  if (-1 == nrd) {
    ERR_PRINT("cannot read journal %s header: %s", src_path->s, strerror(errno));
    err_msg = "read.src_header";
    goto err_read_hdr;
  }

  void *const src_base = mmap(NULL, size, MAP_SHARED, MAP_PRIVATE, src_fd, 0);
  if (MAP_FAILED == src_base) {
    ERR_PRINT("failed in mmap(0, %zd, MAP_SHARED, MAP_PRIVATE, %i, 0): %s", size, src_fd, strerror(errno));
    err_msg = "mmap.src";
    goto err_mmap_src;
  }
  DBG_PRINT("opened source-journal at addr %p", src_base);

  uint64_t off = sizeof(uint64_t);
  const int8_t *src = src_base;

  uint64_t t0 = mg_gettime();

  uint64_t num_msgs = mg_replay_msgs(src, off, size, tbl_names);
  
  uint64_t t1 = mg_gettime();

  if (MG_LOG_DBG_ENABLED) {
    k(0, "{-1 \"replayed \",(string x),\" messages in \",(string 16h$y);}", kj(num_msgs), kj(t1 - t0), 0);
  }

  munmap(src_base, size);
  close(src_fd);

  return kb(1);

err_mmap_src:
err_read_hdr:
  close(src_fd);
  ERR_PRINT("Returning error to KDB");
  return krr(err_msg);
}

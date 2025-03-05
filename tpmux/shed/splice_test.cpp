// from tpmux directory:
//   g++ -std=c++23 -Iinclude -Wall -o bin/splice_test shed/splice_test.cpp
//   bin/splice_test
//
// Essentially, two calls are needed for splice, one for sendfile; splice doesn't seem
// to update the seekable file-offset in the destination file, so lseek is required. 
#ifndef _GNU_SOURCE
#define _GNU_SOURCE         /* See feature_test_macros(7) */
#endif

#define _FILE_OFFSET_BITS 64
#include <fcntl.h>
#include <unistd.h>
#include <sys/sendfile.h>

#include <errno.h>
#include <string.h>

#include "mg_fmt_defs.h"

// https://blogs.oracle.com/linux/post/pipe-and-splice
int main(int argc, char **argv)
{
  int pipe_fd[2] = {0};
  int err = pipe(pipe_fd);
  if (-1 == err) {
    ERR_PRINT("failed in pipe: {}", strerror(errno));
    return 1;
  }

  const char *src = "/home/michaelg/tmp/src_file";
  const char *dst = "/home/michaelg/tmp/dst_file";

  int src_fd = open(src, O_RDONLY);
  if (-1 == src_fd) {
    ERR_PRINT("failed to open file {}: {}", src, strerror(errno));
    return 1;
  }
  INF_PRINT("opened {} as FD {}", src, src_fd);

  int dst_fd = open(dst, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
  if (-1 == dst_fd) {
    ERR_PRINT("failed to open {} for writing: {}", dst, strerror(errno));
    return 1;
  }
  INF_PRINT("opened {} as FD {}", dst, dst_fd);

  ssize_t len;
  loff_t src_off = 0;
  loff_t dst_off = 0;
  len = splice(src_fd, &src_off, pipe_fd[1], nullptr, 10, SPLICE_F_MOVE|SPLICE_F_MORE);
  if (-1 == len) {
    ERR_PRINT("splice-read failed: {}", strerror(errno));
    return 1;
  }
  len = splice(pipe_fd[0], nullptr, dst_fd, &dst_off, 10, SPLICE_F_MOVE|SPLICE_F_MORE);
  if (-1 == len) {
    ERR_PRINT("splice-write failed: {}", strerror(errno));
    return 1;
  }
  INF_PRINT("copied {} bytes from {} to {}", len, src, dst);

  off_t off = lseek(dst_fd, len, SEEK_SET);
  if (-1 == off) {
    ERR_PRINT("failed in lseek: {}", strerror(errno));
    return 1;
  }
  INF_PRINT("set the dst-file offset to {}", off);

  off_t sf_src_off = (off_t)len;
  INF_PRINT("calling sendfile({}, {}, {}, 10)", src_fd, dst_fd, sf_src_off);

  // https://medium.com/swlh/linux-zero-copy-using-sendfile-75d2eb56b39b
  len = sendfile(dst_fd, src_fd, &sf_src_off, 10);
  if (-1 == len) {
    ERR_PRINT("failed in sendfile: {}", strerror(errno));
    return 1;
  }
  INF_PRINT("Copied {} bytes with sendfile", 10);

  return 0;
}

#define _GNU_SOURCE // memfd_create

#include <stdlib.h>     // ExiT_SUCCESS
#include <stddef.h>     // NULL
#include <stdint.h>     // 
#include <stdio.h>      // printf
#include <string.h>     // memset, strnlen
#include <sys/mman.h>   // mmap memfd_create
#include <unistd.h>     // ftruncate, close, read, write
#include <sys/socket.h> // socket, getaddrinfo
#include <sys/types.h>  // getaddrinfo
#include <netdb.h>      // getaddrinfo
#include <errno.h>
#include "k.h"
#include "mg_defs.h"

// open connections to N tickerplants
// open journal file of our own
// open listening port
// 
// receive data
// . strip header, write to journal
// . send to each subscriber; perhaps maintain a per-subscriber buffer to allow some sort of slow consumption
// 
// offer .u.sub semantics, but reply with .u.i and .u.L
struct conn_data
{
	size_t rd_off;
	size_t rd_lim;
	size_t buf_cap;
	void *buf;
	int mem_fd;
	int fd;
};

typedef struct conn_data conn_data_s;


// I first read this as I followed the thread on memory allocation and apart from the friendly sharing of the technique
// one of the things I love the most about it is that I could could find it by searching for "don't be shy".
// The author is Gil Tene: https://groups.google.com/g/mechanical-sympathy/c/_Ws4PDEJ51A/m/9kCnp3zfCPsJ
// 
// "Yes, physical memory for anonymous maps are lazily allocated on Linux (all pages mapped to a zero page until modified),
// but mapped files are different, as they need the actual file storage to exist as backing store. That's why grabbing
// hold of a large amount of contiguous virtual memory upfront with a PROT_NONE is a good idea, as such a mapping will
// never have any physical pages allocated to them, and you can later use that virtual area to grow your file as needed
// without ever needing to cause a temporarily unmapping or lack of concurrent accessibility to any of its contents.
//
// "The way these grow-in-place-as-needed file mmaps are usually done, which is not error prone at all, is to grab the
// max virtual memory amount you can imagine needing (don't be shy) with an anonymous mmap using a PROT_NONE
// protection and an unspecified address. You then take the base address this map comes in at and use it as the base
// for a MAP_FIXED mmap of the file you want, extending with additional  MAP_FIXED mmaps that overlap deeper into the
// anonymous area as needed, making sure the offsets and base addresses of the additional mappings translate to a
// contiguous mapping of the file, and that protection is the same across all the file mmaps. This keeps everything
// safe and under your explicit control. The semantics work out just right, as the kernel will keep other mmaps that
// don't explicitly specify an overlapping address out of your way, but your MAP_FIXED mmaps will happily and forcibly
// replace the previous anonymous mapping at overlapping addresses. Under the hood, the multiple file mmaps do not
// translate into any additional mapping structures (vmas in the kernel) as long as their base addresses, file
// offsets, and protection all fit together correctly such that a single vma can legitimately describe the combined
// mapped file range.
//
// "This trick is regularly used to support on-demand stack extension without requiring executive pre-allocation of
// physical memory for the stack, and can be used just as well for on-demand mmap'ed file extension in this context."
static int init_circ_buffer(conn_data_s *nfo, const size_t buf_len)
{
  void *base = mmap(NULL, 2 * buf_len, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE, -1, 0);
  if (MAP_FAILED == base) {
    ERR_PRINT("Failed to reserve private memory-address range %zd: %s", 2 * buf_len, strerror(errno));
    return -1;
  }

  // Pre-goto variable declarations
  int mfd = -1, err;
  void *tmp = NULL;

  mfd = memfd_create("mg_circ_buf", 0);
  if (-1 == mfd) {
    ERR_PRINT("Failed in memfd_create: %s", strerror(errno));
    goto err_memfd_create;
  }

  err = ftruncate(mfd, buf_len);
  if (-1 == err) {
    ERR_PRINT("Failed in ftruncate: %s", strerror(errno));
    goto err_rest;
  }

  TRA_PRINT("mapped PROT_NONE section at %p, length %zd", (void*)base, buf_len * 2);

  tmp = mmap(base, buf_len, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, mfd, 0);
  if (MAP_FAILED == tmp) {
    ERR_PRINT("Failed in mmap(base, len=%zd, .. MAP_FIXED)", buf_len);
    goto err_rest;
  }

  if (tmp != base) {
    ERR_PRINT("expected MAP_FIXED addr and response to be the same, have addr=%p and result=%p", base, tmp);
    goto err_rest;
  }

  TRA_PRINT("mapped PROT_RW section at %p", (void*)tmp);

  tmp = mmap((char*)base + buf_len, buf_len, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, mfd, 0);
  if (MAP_FAILED == tmp) {
    ERR_PRINT("Failed in mmap(base + buf_len, len=%zd, .. MAP_FIXED)", buf_len);
    goto err_rest;
  }

  if (tmp != ((char*)base + buf_len)) {
    ERR_PRINT("expected MAP_FIXED addr and response to be the same, have addr=%p and result=%p", (void*)((char*)base + buf_len), tmp);
    goto err_rest;
  }

  TRA_PRINT("mapped PROT_RW section at %p", (void*)tmp);

	nfo->buf_cap = buf_len;
	nfo->mem_fd = mfd;

  return 0;
err_rest:
  close(mfd);
err_memfd_create:
  munmap(base, 2 * buf_len);
  return -1;
}

static conn_data_s* alloc_conn_data(size_t buf_cap)
{
	void *tmp = malloc(sizeof(conn_data_s));
	if (NULL == tmp) {
		ERR_PRINT("Failed to allocate (%zd)", sizeof(conn_data_s));
		return NULL;
	}
	memset(tmp, 0, sizeof(conn_data_s));

	conn_data_s *nfo = tmp;

	int err = init_circ_buffer(nfo, buf_cap);
	if (-1 == err) {
		ERR_PRINT("Failed to initialise circular buffer");
		free(tmp);
		return NULL;
	}

	return nfo;
}

#define MG_CBUF_SZ (32)
static conn_data_s* connect_to_tp(const char *host, uint16_t port, size_t buf_cap)
{
	char cbuf[MG_CBUF_SZ] = {0};

	int rtn = snprintf(cbuf, MG_CBUF_SZ, "%hu", port);
	if (rtn >= MG_CBUF_SZ) {
		ERR_PRINT("Failed to convert port value: %hu", port);
		return NULL;
	}

	struct addrinfo hints = {0};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	struct addrinfo *res = NULL;

	int err = getaddrinfo(host, cbuf, &hints, &res);
	if (0 != err) {
		ERR_PRINT("Failed in getaddrinfo: %s", strerror(errno));
		return NULL;
	}

	struct addrinfo *adr;
	int sfd = -1;

	for (adr = res ; NULL != adr ; adr = adr->ai_next) {
		sfd = socket(adr->ai_family, adr->ai_socktype, adr->ai_protocol);
		if (-1 == sfd)
			continue;

		err = connect(sfd, adr->ai_addr, adr->ai_addrlen);
		if (-1 != err) {
			break;
		}
		WRN_PRINT("failed in connect: %s", strerror(errno));
		close(sfd);
		sfd = -1;
	}

	freeaddrinfo(res);

	if (-1 == sfd) {
		ERR_PRINT("Failed to connect to %s:%hu", host, port);
		return NULL;
	}
	INF_PRINT("Connected to KDB instance at %s:%hu", host, port);

	err = getlogin_r(cbuf, MG_CBUF_SZ);
	if (0 != err) {
		ERR_PRINT("Failed in getlogin_r: %s", strerror(errno));
		cbuf[0] = (char)0;
	}
	size_t len = strnlen(cbuf, MG_CBUF_SZ);

	// Update this bit here if you want to send a password to the TP
	if (len > MG_CBUF_SZ - 3) {
		WRN_PRINT("Truncating username to " STRINGIFY(MG_CBUF_SZ) " bytes from %zd", len);
		len = MG_CBUF_SZ - 3;
	}
	
	cbuf[len] = ':';
	// password data would go here, IIRC
	cbuf[len+1] = 3;
	cbuf[len+2] = 0;

	uint32_t tries = 0;
	ssize_t tot = 0;
unpw_retry:
	do {
		ssize_t xwr = write(sfd, cbuf + tot, len + 3 - tot);
		if (-1 == xwr) {
			if (EINTR == errno && tries++ < 3)
				goto unpw_retry;
			ERR_PRINT("Failed to write username & password to TP: %s", strerror(errno));
			close(sfd);
			return NULL;
		}
		tot += xwr;
	}
	while (tot < len + 3);

	tries = 0;
	uint8_t ipc = 0;

ipc_reply:
	{
		ssize_t xrd = read(sfd, &ipc, 1);
		if (-1 == xrd) {
			if (EINTR == errno && tries++ < 3)
				goto ipc_reply;
			ERR_PRINT("Failed to read IPC response-byte from TP: %s", strerror(errno));
		}

		INF_PRINT("Logged-in to KDB instance at %s:%hu as %.*s", host, port, (int)len, cbuf);

		conn_data_s *data = alloc_conn_data(buf_cap);
		data->fd = sfd;
		return data;
	}

}
#undef MG_CBUF_SZ

int tpmux_main(int argc, char **argv)
{
	conn_data_s *data = connect_to_tp("localhost", 30098, 1024 * 128);
	if (NULL == data)
		WRN_PRINT("Failed to connect to TP");
	else
		DBG_PRINT("Connected to TP");

	return EXIT_SUCCESS;
}


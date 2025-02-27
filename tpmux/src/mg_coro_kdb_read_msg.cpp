#include "mg_coro_epoll.h"
#include "mg_coro_task.h"
#include "mg_fmt_defs.h"
#include "KdbType.h"

#include <utility> // std::exchange

namespace mg7x {

Task<mg7x::ReadMsgResult> kdb_read_message(EpollCtl & epoll, int fd, std::vector<int8_t> & ary)
{
  INF_PRINT(YEL "kdb_read_message" RST ": have sock_fd {}", fd);
  if (ary.capacity() < mg7x::SZ_MSG_HDR) {
    ary.reserve(mg7x::SZ_MSG_HDR);
  }
  TRA_PRINT(YEL "kdb_read_message" RST ": ary.capacity is {}", ary.capacity());
  ary.clear();

  EpollCtl::Awaiter awaiter{fd};
  epoll.add_interest(fd, EPOLLIN, awaiter);

  ssize_t tot = 0;

  while (tot < mg7x::SZ_MSG_HDR) {
    TRA_PRINT(YEL "kdb_read_message" RST ": awaiting read-signal for header bytes");
    auto [_fd, rev] = co_await awaiter;
    if (rev & EPOLLERR) {
      ERR_PRINT("error signal from epoll: revents is {}; as yet unhandled, exiting", rev);
      std::terminate();
    }
    TRA_PRINT(YEL "kdb_read_message" RST ": have read-signal");

    ssize_t rdz = read(fd, ary.data(), mg7x::SZ_MSG_HDR);
    TRA_PRINT(YEL "kdb_read_message" RST ": read {} bytes", rdz);
    if (-1 == rdz) {
      ERR_PRINT("failed during read: {}; exiting.", strerror(errno));
      // check EINTR
      std::terminate();
    }
    tot += rdz;
    TRA_PRINT(YEL "kdb_read_message" RST ": read {} bytes in total", tot);
  }

  mg7x::KdbIpcMessageReader rdr{};
  mg7x::ReadMsgResult res{};

  bool dun = rdr.readMsg(ary.data(), mg7x::SZ_MSG_HDR,  res);
  TRA_PRINT(YEL "kdb_read_message" RST ": IPC length is {}, input-bytes consumed {}, msg_type is {}", rdr.getIpcLength(), rdr.getInputBytesConsumed(), res.msg_typ);
  size_t off = 0;
  // doesn't matter for v.3 IPC whether it's compressed or not, bytes [4-7] are the wire-size
  int8_t *dst = ary.data();
  while (!dun) {
    size_t len = std::min(ary.capacity() - off, rdr.getInputBytesRemaining());
    TRA_PRINT(YEL "kdb_read_message" RST ": requesting read of up to {} bytes at offset {}", len, off);

    ssize_t rdz = read(fd, dst + off, len);
    TRA_PRINT(YEL "kdb_read_message" RST ": read {} bytes", rdz);

    if (-1 == rdz) {
      if (EAGAIN == errno) {
        // would have blocked
        auto [_fd, rev] = co_await awaiter;
        // TODO check rev
        continue;
      }
      else if (EINTR == errno) {
        INF_PRINT("placeholder, 'nyi: EINTR on FD {}, exiting", fd);
        std::terminate();
      }
      else {
        ERR_PRINT("failed during read: {}; exiting.", strerror(errno));
        std::terminate();
      }
    }
    size_t hav = off + rdz;
    size_t pre = rdr.getInputBytesConsumed();
    TRA_PRINT(YEL "kdb_read_message" RST ": offering .readMsg {} bytes", hav);
    dun = rdr.readMsg(dst, hav,  res);
    TRA_PRINT(YEL "kdb_read_message" RST ": this read consumed {}, total input bytes used {}, complete? {}", rdr.getInputBytesConsumed() - pre, rdr.getInputBytesConsumed(), dun);

    if (!dun) {
      size_t rem = hav - (rdr.getInputBytesConsumed() - pre);
      if (rem > 0) {
        TRA_PRINT(YEL "kdb_read_message" RST ": compacting data; rem {}, (hav-rem) {}, hav {}", rem, hav-rem, hav);
        // compact
        std::copy(dst + hav - rem, dst + hav, dst);
        off = rem;
      }
      else {
        off = 0;
      }
    }
  }

  TRA_PRINT(YEL "kdb_read_message" RST ": message-read complete, have: \n{}", *res.message.get());
  
  epoll.clr_interest(fd);

  co_return std::move(res);
}

}; // end namespace mg7x
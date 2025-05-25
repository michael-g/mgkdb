#ifndef _mg_coro_domain_obj__H__
#define _mg_coro_domain_obj__H__

#include <stdint.h>

#include <string>
#include <string_view>
#include <vector>
#include <format>

namespace mg7x {

class Subscription
{
  std::string_view m_service;
  std::string_view m_username;
  std::vector<std::string_view> m_tables;

public:
  Subscription(const char *service, const char *username, std::vector<std::string_view> tables)
   : m_service(service)
   , m_username(username)
   , m_tables(tables)
  {
  }

  std::string_view service() const noexcept { return m_service; }
  std::string_view username() const noexcept { return m_username; }
  const std::vector<std::string_view> & tables() const noexcept { return m_tables; }
};

struct KdbIpcLevel
{
  int8_t m_ipc_level;
};

struct TpMsgCounts
{
  uint32_t m_num_msg_total;
  uint32_t m_num_msg_included;
};

class ErrnoMsg
{
  int m_errno;
  std::string_view m_msg;
public:
  ErrnoMsg(int err, std::string_view msg) : m_errno(err), m_msg(msg) {}
  ErrnoMsg() : ErrnoMsg(0, "") {}
  int errnum() const noexcept { return m_errno; }
  std::string_view message() const noexcept { return m_msg; }
};

} // end namespace mg7x

template<>
struct std::formatter<mg7x::ErrnoMsg>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::ErrnoMsg & err, FormatContext & ctx) const
	{
		return std::format_to(ctx.out(), "Error({}, errno={})", err.message(), err.errnum());
	}
};

#endif

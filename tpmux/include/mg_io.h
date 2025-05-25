#ifndef __mg_io__H__
#define __mg_io__H__

#include <string_view>
#include <format> // std::formatter

namespace mg7x::io {

class TcpConn
{
  int m_sock_fd;
  std::string_view m_host;
  std::string_view m_service;
public:
  TcpConn(int sdf, std::string_view host, std::string_view service)
   : m_sock_fd(sdf)
   , m_host(host)
   , m_service(service)
   { }

   TcpConn()
    : TcpConn(-1, "", "")
   {}

   int sock_fd() const noexcept { return m_sock_fd; }

   std::string_view host() const noexcept { return m_host; }

   std::string_view service() const noexcept { return m_service; }

};

} // end namespance mg7x::io

template<>
struct std::formatter<mg7x::io::TcpConn>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	template<class FormatContext> auto format(const mg7x::io::TcpConn & conn, FormatContext & ctx) const
	{
		return std::format_to(ctx.out(), "TcpConn({}:{}, fd={})", conn.host(), conn.service(), conn.sock_fd());
	}
};

#endif

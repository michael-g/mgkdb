#ifndef _mg_coro_domain_obj__H__
#define _mg_coro_domain_obj__H__

#include <string_view>
#include <vector>

namespace mg7x {
class Subscription
{
  std::string_view m_service;
  std::string_view m_username;
  std::vector<std::string_view> m_tables;
  std::string_view m_dst_jnl;

public:
  Subscription(const char *service, const char *username, std::vector<std::string_view> tables, const char *dst_jnl)
   : m_service(service)
   , m_username(username)
   , m_tables(tables)
   , m_dst_jnl(dst_jnl)
  {
  }

  std::string_view service() const noexcept { return m_service; }
  std::string_view username() const noexcept { return m_username; }
  const std::vector<std::string_view> & tables() const noexcept { return m_tables; }
  std::string_view dst_jnl() const noexcept { return m_dst_jnl; }
};

} // end namespace mg7x
#endif

/*
This file is part of the Mg KDB-IPC C++ Library (hereinafter "The Library").

The Library is free software: you can redistribute it and/or modify it under
the terms of the GNU Affero Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

The Library is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU Affero Public License for more details.

You should have received a copy of the GNU Affero Public License along with The
Library. If not, see https://www.gnu.org/licenses/agpl.txt.
*/

#ifndef __MG_FMT_DEFS__
#define __MG_FMT_DEFS__

#include <source_location>
#include <print>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define _MG_TRACE_ 0
#define _MG_DEBUG_ 1
#define _MG_INFO_  2
#define _MG_WARN_  3
#define _MG_ERROR_ 4

#ifndef _MG_LOG_LVL_
#define _MG_LOG_LVL_ _MG_DEBUG_
#endif

/* see https://www.linuxjournal.com/article/8603 for other attributes like 'bright', 'dim' */
/* also https://gist.github.com/JBlond/2fea43a3049b38287e5e9cefc87b2124 */
#define RED   "\x1B[0;91m"
#define GRN   "\x1B[0;92m"
#define YEL   "\x1B[0;93m"
#define BLU   "\x1B[0;94m"
#define MAG   "\x1B[0;95m"
#define CYN   "\x1B[0;96m"
#define WHT   "\x1B[0;97m"
#define BLD   "\x1B[0;98m"
#define RST   "\x1B[0m"

#define DER   "\x1B[41m"
#define NRG   "\x1B[42m"
#define LEY   "\x1B[43m"
#define ULB   "\x1B[44m"
#define GAM   "\x1B[45m"
#define NYC   "\x1B[46m"
#define THW   "\x1B[47m"

#define MG_LOG_TRA_ENABLED (_MG_LOG_LVL_ <= _MG_TRACE_)
#define MG_LOG_DBG_ENABLED (_MG_LOG_LVL_ <= _MG_DEBUG_)
#define MG_LOG_INF_ENABLED (_MG_LOG_LVL_ <= _MG_INFO_)
#define MG_LOG_WRN_ENABLED (_MG_LOG_LVL_ <= _MG_WARN_)
#define MG_LOG_ERR_ENABLED (_MG_LOG_LVL_ <= _MG_ERROR_)

//fprintf(file, lvl ": " fmt " [" MAG "%s" RST "] " BLU "%s" RST ":" CYN TOSTRING(__LINE__) "\n" RST,##__VA_ARGS__, __func__ , loc.file_name()); fflush(file);
#define MG_LOG(file,lvl,fmt,...) { \
  const std::source_location loc = std::source_location::current(); \
  std::print(lvl ": " fmt " [" MAG "{}" RST "] " BLU "{}" RST ":" CYN TOSTRING(__LINE__) "\n" RST,##__VA_ARGS__, __func__, loc.function_name()); \
}
//#define LOG(file,lvl,fmt,...)      { std::print(file, lvl ": " fmt " [" MAG "{}" RST "] " BLU  __FILE__ RST ":" CYN TOSTRING(__LINE__) "\n" RST,##__VA_ARGS__, __func__ ); fflush(file); }
#define TRA_PRINT(fmt, ...)        do { if (MG_LOG_TRA_ENABLED) MG_LOG(stdout, BLU "TRACE" RST, fmt RST,##__VA_ARGS__)} while (0)
#define DBG_PRINT(fmt, ...)        do { if (MG_LOG_DBG_ENABLED) MG_LOG(stdout, MAG "DEBUG" RST, fmt RST,##__VA_ARGS__)} while (0)
#define INF_PRINT(fmt, ...)        do { if (MG_LOG_INF_ENABLED) MG_LOG(stdout, CYN " INFO" RST, fmt RST,##__VA_ARGS__)} while (0)
#define WRN_PRINT(fmt, ...)        do { if (MG_LOG_WRN_ENABLED) MG_LOG(stdout, YEL " WARN" RST, fmt RST,##__VA_ARGS__)} while (0)
#define ERR_PRINT(fmt, ...)        do { if (MG_LOG_ERR_ENABLED) MG_LOG(stdout, RED "ERROR" RST, fmt RST,##__VA_ARGS__)} while (0)

#endif

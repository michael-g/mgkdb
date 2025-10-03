/* This file is part of the "Mg kdb+ Library" (hereinafter "The Library").
 *
 * The Library is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * The Library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Affero Public License for more details.
 *
 * You should have received a copy of the GNU Affero Public License along with The
 * Library. If not, see https://www.gnu.org/licenses/agpl.txt.
 */
#ifndef MG_INC_MG_DEBUG_H
#define MG_INC_MG_DEBUG_H

#include <source_location>
#include <string_view>
#include <print>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define MG_LOG_LVL_TRACE 0
#define MG_LOG_LVL_DEBUG 1
#define MG_LOG_LVL_INFO  2
#define MG_LOG_LVL_WARN  3
#define MG_LOG_LVL_ERROR 4

#ifndef MG_LOG_LVL
#define MG_LOG_LVL MG_LOG_LVL_DEBUG
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

#define MG_LOG_TRA_ENABLED (MG_LOG_LVL <= MG_LOG_LVL_TRACE)
#define MG_LOG_DBG_ENABLED (MG_LOG_LVL <= MG_LOG_LVL_DEBUG)
#define MG_LOG_INF_ENABLED (MG_LOG_LVL <= MG_LOG_LVL_INFO)
#define MG_LOG_WRN_ENABLED (MG_LOG_LVL <= MG_LOG_LVL_WARN)
#define MG_LOG_ERR_ENABLED (MG_LOG_LVL <= MG_LOG_LVL_ERROR)

//fprintf(file, lvl ": " fmt " [" MAG "%s" RST "] " BLU "%s" RST ":" CYN TOSTRING(__LINE__) "\n" RST,##__VA_ARGS__, __func__ , loc.file_name()); fflush(file);
#define MG_LOG(file,lvl,fmt,...) do { \
  const std::source_location loc = std::source_location::current(); \
  std::string_view file_name{loc.file_name()}; \
  std::print(lvl ": " fmt " " BLU "{}" RST ":" CYN TOSTRING(__LINE__) RST " [" MAG "{}" RST "]" "\n",##__VA_ARGS__, loc.file_name() + file_name.find_last_of('/') + 1, __func__); \
} while(0)
//#define LOG(file,lvl,fmt,...)      { std::print(file, lvl ": " fmt " [" MAG "{}" RST "] " BLU  __FILE__ RST ":" CYN TOSTRING(__LINE__) "\n" RST,##__VA_ARGS__, __func__ ); fflush(file); }
#define TRA_PRINT(fmt, ...)        do { if (MG_LOG_TRA_ENABLED) MG_LOG(stdout, BLU "TRACE" RST, fmt RST,##__VA_ARGS__);} while (0)
#define DBG_PRINT(fmt, ...)        do { if (MG_LOG_DBG_ENABLED) MG_LOG(stdout, MAG "DEBUG" RST, fmt RST,##__VA_ARGS__);} while (0)
#define INF_PRINT(fmt, ...)        do { if (MG_LOG_INF_ENABLED) MG_LOG(stdout, CYN " INFO" RST, fmt RST,##__VA_ARGS__);} while (0)
#define WRN_PRINT(fmt, ...)        do { if (MG_LOG_WRN_ENABLED) MG_LOG(stdout, YEL " WARN" RST, fmt RST,##__VA_ARGS__);} while (0)
#define ERR_PRINT(fmt, ...)        do { if (MG_LOG_ERR_ENABLED) MG_LOG(stderr, RED "ERROR" RST, fmt RST,##__VA_ARGS__);} while (0)

#endif

#ifndef __MG_DEFS__

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
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RST   "\x1B[0m"

#define MG_LOG_TRA_ENABLED (_MG_LOG_LVL_ <= _MG_TRACE_)
#define MG_LOG_DBG_ENABLED (_MG_LOG_LVL_ <= _MG_DEBUG_)
#define MG_LOG_INF_ENABLED (_MG_LOG_LVL_ <= _MG_INFO_)
#define MG_LOG_WRN_ENABLED (_MG_LOG_LVL_ <= _MG_WARN_)
#define MG_LOG_ERR_ENABLED (_MG_LOG_LVL_ <= _MG_ERROR_)

#define LOG(file,lvl,fmt,...)      { fprintf(file, lvl ": " fmt " [" MAG "%s" RST "] " BLU  __FILE__ RST ":" CYN TOSTRING(__LINE__) "\n" RST,##__VA_ARGS__, __func__ ); fflush(file); }
#define TRA_PRINT(fmt, ...)        do { if (MG_LOG_TRA_ENABLED) LOG(stdout, BLU "TRACE" RST, fmt RST,##__VA_ARGS__)} while (0)
#define DBG_PRINT(fmt, ...)        do { if (MG_LOG_DBG_ENABLED) LOG(stdout, MAG "DEBUG" RST, fmt RST,##__VA_ARGS__)} while (0)
#define INF_PRINT(fmt, ...)        do { if (MG_LOG_INF_ENABLED) LOG(stdout, CYN " INFO" RST, fmt RST,##__VA_ARGS__)} while (0)
#define WRN_PRINT(fmt, ...)        do { if (MG_LOG_WRN_ENABLED) LOG(stdout, YEL " WARN" RST, fmt RST,##__VA_ARGS__)} while (0)
#define ERR_PRINT(fmt, ...)        do { if (MG_LOG_ERR_ENABLED) LOG(stdout, RED "ERROR" RST, fmt RST,##__VA_ARGS__)} while (0)

#endif

#ifndef __LOG_H__
#define __LOG_H__

void printf(char *, ...);

// Please use one of these

// #define LOG_LEVEL_NONE
#define LOG_LEVEL_CRITICAL
// #define LOG_LEVEL_DEBUG
#define LOG_LEVEL_INFO
// #define LOG_LEVEL_TRACE
// #define LOG_LEVEL_ALL

#if defined(LOG_LEVEL_CRITICAL)

#define USE_LOG_ERROR
#define USE_LOG_WARN

#endif // LOG_LEVEL_CRITICAL

#if defined(LOG_LEVEL_DEBUG)

#define USE_LOG_ERROR
#define USE_LOG_WARN
#define USE_LOG_DEBUG

#endif // LOG_LEVEL_DEBUG

#if defined(LOG_LEVEL_INFO)

#define USE_LOG_INFO

#endif // LOG_LEVEL_INFO

#if defined(LOG_LEVEL_TRACE)

#define USE_LOG_INFO
#define USE_LOG_TRACE

#endif // LOG_LEVEL_TRACE

#if defined(LOG_LEVEL_ALL)

#define USE_LOG_WARN
#define USE_LOG_ERROR
#define USE_LOG_INFO
#define USE_LOG_DEBUG
#define USE_LOG_TRACE

#endif // LOG_LEVEL_ALL

enum LOG_COLOR {
    RED = 31,
    GREEN = 32,
    BLUE = 34,
    GRAY = 90,
    YELLOW = 93,
};

extern int debug_core_color[];

#if defined(USE_LOG_WARN)

#define warnf(fmt, ...) printf("\x1b[%dm[%s] " fmt "\x1b[0m\n", YELLOW, "WARN", ##__VA_ARGS__)
#else
#define warnf(fmt, ...)
#endif //

#if defined(USE_LOG_ERROR)

#define errorf(fmt, ...)                                                                                          \
    do {                                                                                                          \
        int hartid = cpuid();                                      \
        printf("\x1b[%dm[%s %d] %s:%d: " fmt "\x1b[0m\n", RED, "ERROR", hartid, __FILE__, __LINE__, ##__VA_ARGS__);\
        printtrace();   \
    } while (0)
#else
#define errorf(fmt, ...)
#endif //

#if defined(USE_LOG_DEBUG)

#define debugf(fmt, ...) printf("\x1b[%dm[%s] " fmt "\x1b[0m\n", GREEN, "DEBUG", ##__VA_ARGS__)

#define debugcore(fmt, ...)                                                                                   \
    do {                                                                                                      \
        int hartid = cpuid();                                                                              \
        printf("\x1b[%dm[%s %d] " fmt "\x1b[0m\n", debug_core_color[hartid], "DEBUG", hartid, ##__VA_ARGS__); \
    } while (0)

// print var in hex
#define phex(var_name) debugf(#var_name "=%p", var_name)

#else
#define debugf(fmt, ...)
#define debugcore(fmt, ...)
#define phex(var_name) (void)(var_name);
#endif //

#if defined(USE_LOG_TRACE)

#define tracef(fmt, ...) printf("\x1b[%dm[%s] " fmt "\x1b[0m\n", GRAY, "TRACE", ##__VA_ARGS__)

#define tracecore(fmt, ...)                                                                             \
    do {                                                                                                \
        uint64 hartid = cpuid();                                                                        \
        printf("\x1b[%dm[TRACE %d] " fmt "\x1b[0m\n", debug_core_color[hartid], hartid, ##__VA_ARGS__); \
    } while (0)
#else
#define tracef(fmt, ...)
#define tracecore(fmt, ...)
#endif //

#if defined(USE_LOG_INFO)

#define infof(fmt, ...) printf("\x1b[%dm[%s] " fmt "\x1b[0m\n", BLUE, "INFO", ##__VA_ARGS__)
#else
#define infof(fmt, ...)
#endif //

#endif //!__LOG_H__

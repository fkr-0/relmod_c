// log.h - only included once per file, uses LOG_PREFIX
#ifndef LOG_H
#define LOG_H

#ifdef MENU_DEBUG
#ifndef LOG_PREFIX
#define LOG_PREFIX "[GENERIC]"
#endif
#define LOG(fmt, ...) fprintf(stderr, LOG_PREFIX " " fmt "\n", ##__VA_ARGS__)
#else
#define LOG(fmt, ...) ((void)0)
#endif
#endif // LOG_H


#ifndef LOGGER_H
#define	LOGGER_H
/*LOG levels*/
#define LOG_ALWAYS -1
#define LOG_NONE 0
#define LOG_ERR 1
#define LOG_WARN 2
#define LOG_INFO 3
#define LOG_DEBUG 4
#define LOG_ALL 5
/*Log buffer*/
#define LOG_BUFFER_SIZE 1024

extern int log_level;
extern int verbose_level;

void init_logger(char *filename);
int log_line(char *msg, int severity);
void raise_error(char *msg);
void stop_logger();

#endif	/* LOGGER_H */


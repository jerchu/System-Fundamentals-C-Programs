#ifndef SFISH_H
#define SFISH_H

/* Format Strings */
#define EXEC_NOT_FOUND "sfish: %s: command not found\n"
#define BUILTIN_ERROR  "sfish builtin error: %s\n"
#define SYNTAX_ERROR   "sfish syntax error: %s\n"
#define EXEC_ERROR     "sfish exec error: %s\n"
#define JOBS_LIST_ITEM "[%d] %s\n"
#define STRFTIME_RPRMT "%a %b %e, %I:%M%p"

/* redirection constants */
#define NO_REDIRECTION 0
#define PIPING_REDIRECTION 1
#define INPUT_REDIRECTION 2
#define OUTPUT_REDIRECTION 3

/* colors */
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_WHITE   "\x1b[37m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_COLOR_BROWN   "\x1b[38;5;130m"


#endif

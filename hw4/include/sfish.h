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
#define ANSI_COLOR_RED     "\001\x1b[31m\002"
#define ANSI_COLOR_GREEN   "\001\x1b[32m\002"
#define ANSI_COLOR_YELLOW  "\001\x1b[33m\002"
#define ANSI_COLOR_BLUE    "\001\x1b[34m\002"
#define ANSI_COLOR_MAGENTA "\001\x1b[35m\002"
#define ANSI_COLOR_CYAN    "\001\x1b[36m\002"
#define ANSI_COLOR_WHITE   "\001\x1b[37m\002"
#define ANSI_COLOR_RESET   "\001\x1b[0m\002"
#define ANSI_COLOR_BROWN   "\001\x1b[38;5;130m\002"


#endif

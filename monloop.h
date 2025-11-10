#ifndef __monloop_h__
#define __monloop_h__

#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>

#define MONLOOP_LINELEN 4096

typedef struct monloop {
  char               line[MONLOOP_LINELEN]; // line buffer
  void              *data;                  // user data pointer
  FILE              *fp;                    // filepointer for fprintf calls
  pthread_t          tid;                   // monitor thread
  pthread_barrier_t  barrier;	            // barrier for thread start
  int                fd;                    // file descriptor for monitor i/o 
  int                n;                     // number of bytes buffered
  bool               silent;                // silent do produce output
  bool               running;               // true after mon thread started         
} monloop_t;

#define ML_PRINT(this, fmt, ...) {				\
  if (!this->silent) fprintf(this->fp, fmt, __VA_ARGS__);	\
  }

int monloop_start(monloop_t *this, void *data, int fd, FILE *fp, bool silent);
int monloop_join(monloop_t *this, void **exit_status);

typedef long (*monloop_cmd_t)(monloop_t *, int, int);

#define MONLOOP_CMD_EXIT   -1
#define MONLOOP_CMD_OK      1
#define MONLOOP_CMD_FAILED  0

typedef struct monloop_cmddesc {
  char *name;
  char *usage;
  monloop_cmd_t cmd;
} monloop_cmddesc_t;

long monloop_help_cmd(monloop_t *this, int args, int epollfd);

extern monloop_cmddesc_t monloop_cmds[];
#endif

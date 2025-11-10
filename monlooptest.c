#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "monloop.h"

struct AppData {
  int value;
  struct MyData *next;
};

static long my_exit_cmd(monloop_t *this, int args, int epollfd) {
  struct AppData *data = this->data;
  ML_PRINT(this, "Goodbye cruel world: %d\n", data->value);
  return MONLOOP_CMD_EXIT;
}

static long my_display_cmd(monloop_t *this, int args, int epollfd) {
  struct AppData *data = this->data;
  ML_PRINT(this, "value: %d next:%p\n", data->value, data->next);
  return MONLOOP_CMD_OK;
}

static long my_add_cmd(monloop_t *this, int args, int epollfd) {
  struct AppData *data = this->data;
  
  if (args == 0) {
    ML_PRINT(this, "%s", "USAGE: add <value>\n");
    return MONLOOP_CMD_FAILED;
  }

  char *argstr = &(this->line[args]);
  int val = atoi(argstr);
  data->value += val;
  
  return MONLOOP_CMD_OK;
}

monloop_cmddesc_t monloop_cmds[] = {
  	{ .name = "help", .usage = "print help for monitor commands", .cmd = monloop_help_cmd },
	{ .name = "add",  .usage = "add <value>", .cmd = my_add_cmd },
	{ .name = "+", .usage = NULL, .cmd = my_add_cmd },
	{ .name = "display", .usage = "display app data", .cmd = my_display_cmd },
	{ .name = "d", .usage = NULL, .cmd = my_display_cmd },
	{ .name = "exit", .usage = "cleanup and exit", .cmd = my_exit_cmd },
	{ .name = "e", .usage = NULL, .cmd = my_exit_cmd },
	{ .name = "quit", .usage = "cleanup and exit", .cmd = my_exit_cmd },
	{ .name = "q", .usage = NULL, .cmd = my_exit_cmd },
	{ NULL, NULL, NULL }
};

monloop_t monloop;

int
main() {
  long rc;
  void *exit_status;
  struct AppData data = {
    .value = 42,
    .next = NULL
  };
  
  printf("monloop test\n");
  monloop_start(&monloop, &data, STDIN_FILENO, stderr, false);
  
  monloop_join(&monloop, &exit_status);
  rc = (long)exit_status;
  printf("monloop exited with rc=%ld\n", rc);
  return rc;
}

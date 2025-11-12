#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <sys/timerfd.h>
#include <errno.h>

#include "monloop.h"

struct AppData {
  int value;
  monloop_timer_t *tmr;
};

static long tmr(monloop_timer_t *timer, int evnts)
{
  uint64_t num;
  monloop_t *monloop = timer->monloop;
  struct AppData *data = monloop->data;
  
  int n = read(timer->fd,&num,sizeof(num)); // ack the timer
  assert(n!=-1);
  assert(num>0);

  data->value += 1;
  //  ML_PRINT(monloop, "Timer fired %ld\n",num);
  return MONLOOP_CMD_OK;
}

static long my_exit_cmd(monloop_t *this, int args) {
  struct AppData *data = this->data;
  ML_PRINT(this, "Goodbye cruel world: %d\n", data->value);
  return MONLOOP_CMD_EXIT;
}

static long my_display_cmd(monloop_t *this, int args) {
  struct AppData *data = this->data;
  ML_PRINT(this, "value: %d tmr:%p\n", data->value, data->tmr);
  return MONLOOP_CMD_OK;
}

static long my_add_cmd(monloop_t *this, int args) {
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

static long my_set_timer_cmd(monloop_t *this, int args) {
  struct AppData *data = this->data;
  monloop_timer_t *tmr = data->tmr;
  char *argstr;
  long sec, nsec, isec, insec;
  
  if (tmr == NULL) {
    ML_PRINT(this, "%s", "No timer exists\n");
    return MONLOOP_CMD_FAILED;
  }

  if (args == 0) {
    ML_PRINT(this, "%s", "USAGE: settmr <sec> <nsec> <isec> <insec>\n");
    return MONLOOP_CMD_FAILED;
  }
  
  argstr = &(this->line[args]);
  int n = sscanf(argstr, "%ld %ld %ld %ld", &sec, &nsec, &isec, &insec);
  if (n != 4) {
    ML_PRINT(this, "%s", "USAGE: settmr <sec> <nsec> <isec> <insec>\n");
    return MONLOOP_CMD_FAILED;
  }
  
  // set the timer to fire every second 
  int rc = monloop_settimer(tmr, sec, nsec, isec, insec);
  if (rc == -1) {
    ML_PRINT(this, "monloop_settime failed errno=%d\n", errno);
    return MONLOOP_CMD_FAILED;
  }
  
  return MONLOOP_CMD_OK;
}

static long my_new_timer_cmd(monloop_t *this, int args) {
  struct AppData *data = this->data;
  if (data->tmr != NULL) {
    ML_PRINT(this, "%s", "Timer already exists\n");
    return MONLOOP_CMD_FAILED;
  }
  data->tmr = monloop_addtimer(this, CLOCK_MONOTONIC, tmr);
  assert(data->tmr != NULL);
  return MONLOOP_CMD_OK;
}

monloop_cmddesc_t monloop_cmds[] = {
  	{ .name = "help", .usage = "print help for monitor commands", .cmd = monloop_help_cmd },
	{ .name = "add",  .usage = "add value", .cmd = my_add_cmd },
	{ .name = "+", .usage = NULL, .cmd = my_add_cmd },
	{ .name = "newtmr",  .usage = "create a new timer", .cmd = my_new_timer_cmd },
	{ .name = "settmr",  .usage = "set timer", .cmd = my_set_timer_cmd },
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
    .value = 0,
    .tmr   = NULL,
  };
  
  printf("monloop test\n");
  monloop_start(&monloop, &data, STDIN_FILENO, stderr, false);
  
  monloop_join(&monloop, &exit_status);
  rc = (long)exit_status;
  printf("monloop exited with rc=%ld\n", rc);
  return rc;
}

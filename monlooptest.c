#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
#include <sys/timerfd.h>
#include <errno.h>

#include "monloop.h"

struct Event {
  uint64_t sec;
  uint64_t nsec;
  char     action;
  int      value;
};

struct AppData {
  monloop_timer_t *tmr;
  char *eventdata;
  char *eventnext;
  size_t eventdatalen;
  struct Event e;
  int lambda;
};

static int parseEvent(struct AppData *data)
{
  char *line = data->eventnext;
  struct Event *e = &data->e;
  int len;
  int n = sscanf(line, "%" SCNu64 "%" SCNu64 " %c %d\n%n",
		 &e->sec, &e->nsec, &e->action, &e->value, &len);
  if (n != 4) return 0;
  line += len;
  data->eventnext = line;
  return 1;
}

static long tmr(monloop_timer_t *timer, int evnts)
{
  uint64_t num;
  monloop_t *monloop = timer->monloop;
  struct AppData *data = monloop->data;
  
  int n = read(timer->fd,&num,sizeof(num)); // ack the timer
  assert(n!=-1);
  assert(num>0);

  data->lambda += 1;
  //  ML_PRINT(monloop, "Timer fired %ld\n",num);
  return MONLOOP_CMD_OK;
}

static long event(monloop_timer_t *timer, int evnts)
{
  uint64_t num;
  monloop_t *monloop = timer->monloop;
  struct AppData *data = monloop->data;
  struct Event *e = &data->e;
  
  int n = read(timer->fd,&num,sizeof(num)); // ack the timer
  assert(n!=-1);
  assert(num>0);

  // process event
  char action;
  int value;
  action = e->action;
  value  = e->value;
  
  ML_PRINT(monloop, "Event: action:%c value:%d\n", action, value);
  
  switch (action) {
  case '+':
    data->lambda += value;
  case '=':
    data->lambda = value;
    break;
  default:	
    ML_PRINT(monloop, "unknown event action:%c\n", action);
  }
  // get next event
  int rc = parseEvent(data);
  if (rc == 0) {
    // no more events
    monloop_settimer(timer, 0,0,0,0); // disarm timer
    monloop_unmapfile(data->eventdata, data->eventdatalen);
    data->eventdata = NULL;
    data->eventnext = NULL;
    data->eventdatalen = 0;
    monloop_removetimer(monloop, timer);
    data->tmr = NULL;
    ML_PRINT(monloop, "%s", "End of event file reached\n");
  } else { 
    rc = monloop_settimer(timer, data->e.sec, data->e.nsec, 0 , 0);
    if (rc == -1) {
      ML_PRINT(monloop, "monloop_settime failed errno=%d\n", errno);
      return MONLOOP_CMD_FAILED;
    }
  }
  return MONLOOP_CMD_OK;
}

static long my_exit_cmd(monloop_t *this, int args) {
  struct AppData *data = this->data;
  ML_PRINT(this, "Goodbye cruel world: %d\n", data->lambda);
  return MONLOOP_CMD_EXIT;
}

static long my_display_cmd(monloop_t *this, int args) {
  struct AppData *data = this->data;
  ML_PRINT(this, "lambda: %d tmr:%p eventdata:%p eventnext:%p\n",
	   data->lambda, data->tmr, data->eventdata, data->eventnext);
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
  data->lambda += val;
  
  return MONLOOP_CMD_OK;
}


static long my_play_cmd(monloop_t *this, int args) {
  struct AppData *data = this->data;
  int rc;
  
  if (args == 0) {
    ML_PRINT(this, "%s", "USAGE: play <eventfile>\n");
    return MONLOOP_CMD_FAILED;
  }

  if (data->tmr != NULL) {
    ML_PRINT(this, "%s", "timer in use\n");
    return MONLOOP_CMD_FAILED;
  }

  if (data->eventdata != NULL) {
    ML_PRINT(this, "%s", "busy playing an event file already\n");
    return MONLOOP_CMD_FAILED;
  }
  
  char *file = &(this->line[args]);

  rc = monloop_mapfile(file,
		       (void **)&(data->eventdata),
		       &(data->eventdatalen));

  if (rc == -1) {
    ML_PRINT(this, "monloop_mapfile failed for file:%s\n", file);
    return MONLOOP_CMD_FAILED;
  }

  data->eventnext = data->eventdata;
  rc = parseEvent(data);
  if (rc == 0) {
    ML_PRINT(this, "no events found in file:%s\n", file);
    return MONLOOP_CMD_FAILED;
  }

  data->tmr = monloop_addtimer(this, CLOCK_MONOTONIC, event);
  assert(data->tmr != NULL);  

  rc = monloop_settimer(data->tmr, data->e.sec, data->e.nsec, 0 , 0);
  if (rc == -1) {
    ML_PRINT(this, "monloop_settime failed errno=%d\n", errno);
    return MONLOOP_CMD_FAILED;
  }

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

static long my_del_timer_cmd(monloop_t *this, int args) {
  struct AppData *data = this->data;
  if (data->tmr == NULL) {
    ML_PRINT(this, "%s", "No Timer\n");
    return MONLOOP_CMD_FAILED;
  }
  int rc = monloop_removetimer(this, data->tmr);
  data->tmr = NULL;
  
  assert(rc==1);
  return MONLOOP_CMD_OK;
}

monloop_cmddesc_t monloop_cmds[] = {
  	{ .name = "help", .usage = "print help for monitor commands", .cmd = monloop_help_cmd },
	{ .name = "add",  .usage = "add value", .cmd = my_add_cmd },
	{ .name = "+", .usage = NULL, .cmd = my_add_cmd },
	{ .name = "newtmr",  .usage = "create a new timer", .cmd = my_new_timer_cmd },
	{ .name = "deltmr",  .usage = "delete the timer", .cmd = my_del_timer_cmd },
	{ .name = "settmr",  .usage = "set timer", .cmd = my_set_timer_cmd },
	{ .name = "play",  .usage = "play event file", .cmd = my_play_cmd },
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
    .lambda = 0,
    .tmr   = NULL,
    .eventdata = NULL,
    .eventnext = NULL,
    .eventdatalen = 0,
    .e = {.sec = 0, .nsec = 0, .action = 0, .value = 0}
  };
  
  printf("monloop test\n");
  monloop_start(&monloop, &data, STDIN_FILENO, stderr, false);
  
  monloop_join(&monloop, &exit_status);
  rc = (long)exit_status;
  printf("monloop exited with rc=%ld\n", rc);
  return rc;
}

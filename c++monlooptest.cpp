#include "monloop.h"
#include <unistd.h>
#include <iostream>

class AppData {
private:
  int lambda_;
public:
  AppData() : lambda_(0) {}
  void display() { std::cout << lambda_ << std::endl; }
};

long display_cmd(monloop_t *ml, int args) {
  AppData *ad = static_cast<AppData *>(ml->data);
  ad->display();
  return MONLOOP_CMD_OK;
}

monloop_t monloop;

monloop_cmddesc_t monloop_cmds[] = {
  { .name = "help", .usage = "print help for monitor commands", .cmd = monloop_help_cmd },
  { .name = "display",  .usage = "display", .cmd = display_cmd },
  { NULL, NULL, NULL }
};


int main()
{
  void *exit_status;
  AppData *mdata = new AppData;

  monloop_start(&monloop, mdata, STDIN_FILENO, stderr, false);
  monloop_join(&monloop, &exit_status);
  long rc = (long)exit_status;
  std::cout << "monloop exited with rc=" <<  rc << std::endl;
  
  return rc;
}



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
//#include <math.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

#include "test_runner.h"

int result_pre(TESTDEVICE_HANDLER* deviceParam)
{
  printf("sleep ------------------\n");
  sleep(1);
  return 0;
}


/*------------------------------------------------------------*/
static TestRunnerProcess test_procs = {
    
    result_pre,
    NULL
};



int main(int argc, char** argv)
{
  test_runner_create("./async_mode_cfg.json",&test_procs);

  test_runner_run();
}


#include "test_runner.h"


/*------------------------------------------------------------*/
static TestRunnerProcess test_procs = {
    
    NULL,
    NULL
};



int main(int argc, char** argv)
{
  test_runner_create("./plugins/ncsdk_204/config/movidius_sync.json",&test_procs);

  test_runner_run();
}

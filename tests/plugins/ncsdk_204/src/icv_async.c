
#include "test_runner.h"


int main(int argc, char** argv)
{
  test_runner_create("./plugins/ncsdk_204/config/movidius_async.json",NULL);

  test_runner_run();
}

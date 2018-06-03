#!/usr/bin/env python
import sys, os, re, string,subprocess,time
import shlex 

import getopt
import json

from collections import deque
from operator import itemgetter
import tarfile
from multiprocessing import Process
from multiprocessing import Pool


def test(index):

  log = subprocess.check_output(['sudo','./pc/vscHost',strI])
  print("\033[1;33;44mStart test myx %d ......\033[0m\n" % (index,))
  strI= str(index)
  log = log.strip()
  print log
  lines = log.split("\n")
  lines_len = len(lines)
  if cmp(lines[lines_len-1],"test ok")==0:
    print("\033[1;33;44mmyx %s test finished, test passed\033[0m\n" % (strI,))
    results.append(True)
  else:
    print("\033[1;33;44mmyx %s test finished, test failed\033[0m\n" % (strI,))



def run_all():

  results = []
  log = subprocess.check_output(['./bsl_reset'])
  time.sleep(2)

  for i in range(8):
  	log = subprocess.check_output(['./moviUsbBoot','./myriad/myriad.mvcmd'])
  
  time.sleep(1)

  log = subprocess.check_output(['lsusb','-d','040e:f63b'])
  
  log=log.strip()
  print log
  devices = log.split('\n')
  assert(len(devices)==8);
  print("\033[1;33;44mBootup %d myx\033[0m\n" % (len(devices),))

  pool = Pool(processes=8)
  for i in xrange(10):
    
    pool.apply_async(test, (i, ))
  pool.close()
  pool.join()



  
def run(index):

  log = subprocess.check_output(['./bsl_reset'])
  time.sleep(2)

  for i in range(8):
    log = subprocess.check_output(['./moviUsbBoot','./myriad/myriad.mvcmd'])
  
  time.sleep(1)

  log = subprocess.check_output(['lsusb','-d','040e:f63b'])
  
  log=log.strip()
  print log
  devices = log.split('\n')
  assert(len(devices)==8);
  print("\033[1;33;44mBootup %d myx\033[0m\n" % (len(devices),))

  print("\033[1;33;44mStart test myx %s....... \033[0m\n" % (index,))
  #strI= str(i)
  log = subprocess.check_output(['sudo','./pc/vscHost',index])
  log=log.strip()
  print log
  #print("\033[1;33;44mmyx %s test finished, please check above summary\033[0m\n" % (index,))
  lines = log.split("\n")
  lines_len= len(lines)
  if cmp(lines[lines_len-1],"test ok")==0:
    print("\033[1;33;44mmyx %s test finished, test passed\033[0m\n" % (index,))
  else:
    print("\033[1;33;44mmyx %s test finished, test failed\033[0m\n" % (index,))


def usage():
  print("python usb_test.py")

def main():
  try:
    opts, args = getopt.getopt(sys.argv[1:], "hai:", ["help", "all","index="])

  except getopt.GetoptError as err:
    print (err) # will print something like "option -a not recognized"
    usage()
    sys.exit(2)

  who = os.popen("whoami").read()
  pwd = os.popen("pwd").read()
  print("I am %s" % (who,))
  print("pwd %s" % (pwd,))
  
  for o, a in opts:
    if o in ("-h", "--help"):
      usage()
      sys.exit()
    elif o in ("-i", "--index"):
      index = a
      run(index)
    elif o in ("-a", "--all"):
      run_all()
    else:
    	run_all()



if __name__ == '__main__':
  main()
#!/usr/bin/env python
import sys, os, re, string,subprocess
import shlex 

import getopt
import json

from collections import deque
from operator import itemgetter
import tarfile


def install_drv(folder,name,target):
  print("\033[1;33;44minstall_drv %s\033[0m" % (name,))
  log = subprocess.check_output(['make', target],cwd=folder)
  print log

def install_drv_ion(target):
  install_drv("./drv_ion","myd_ion",target)

def install_drv_vsc(target):
  install_drv("./drv_vsc","myd_vsc",target)

def usage():
  print "python install_drv.py"

def main():
  try:
    opts, args = getopt.getopt(sys.argv[1:], "htim:u", ["help", "test","install","uninstall","module="])

  except getopt.GetoptError as err:
    print str(err) # will print something like "option -a not recognized"
    usage()
    sys.exit(2)

  who = os.popen("whoami").read()
  pwd = os.popen("pwd").read()
  print("I am %s" % (who,))
  print("pwd %s" % (pwd,))
  module = ""
  for o, a in opts:
    if o in ("-h", "--help"):
      usage()
      sys.exit()
    elif o in ("-i", "--install"):
      install_drv_ion("install")
      install_drv_vsc("install")
    elif o in ("-u", "--uninstall"):
      install_drv_ion("uninstall")
      install_drv_vsc("uninstall")
    elif o in ("-m", "--module"):
      module = a
    else:
      assert False, "unhandled option"

  if module != "":
    print "compile module %s" % (module,)
    if cmp(module,"ion") == 0:
      install_drv_ion("install")
    elif cmp(module,"vsc") == 0:
      install_drv_vsc("install")


if __name__ == '__main__':
  main()

#!/usr/bin/env python
# -*- coding: utf-8 -*-
from component import Component
import sys, os, re, string,subprocess,time

import threading
import logging

timer = None 
module_logger = logging.getLogger('ion')

def check_memory_usage():
	global module_logger
	log = os.popen('sudo cat /sys/kernel/debug/ion/heaps/dmaalloc').read()
	log=log.strip()
	module_logger.info(log)
	global timer
	timer = threading.Timer(5.5, check_memory_usage)
	timer.start()

class ION(Component):
	def __init__(self, name="ion"):
		super(ION, self).__init__(name)


	def check_ion_drv(self):
	  log = os.popen('lsmod | grep myd_ion').read()
	  log=log.strip()
	  if len(log) is 0:
	    print("\033[1;33;91m[Error]myd_ion is not installed, try sudo insmod myd_ion.ko in icv-fathom \033[0m\n" )
	    return False
	  else:
	    print("\033[1;33;94m[Succ]myd_ion is installed, passed\033[0m\n" )
	    return True

	def check_group(self):
	  log = os.popen('groups | grep users').read()
	  log=log.strip()
	  if len(log) is 0:
	    self.logger.error("\033[1;33;91m[Error]Current User is not in group 'users', try groupadd users\033[0m\n" )
	    return False
	  else:
	    self.logger.info("\033[1;33;94m[Succ]Current User is in group 'users', passed\033[0m\n" )
	    return True


	def check_udev(self):
	  rules_dir = '/etc/udev/rules.d'
	  if not os.path.isdir(rules_dir):
            self.logger.error("\033[1;33;91m[Error]/etc/udev/rules.d does not exist!\033[0m\n")
            return False
	  names = os.listdir(rules_dir)
	  
	  matched_paths = []
	  matched_lines = []
	  for name in names:
	    if name.endswith(".rules"):
	      fullpath= os.path.join('/etc/udev/rules.d',name)
	      #print fullpath
	      with open(fullpath,'rt') as f:
	        for line in f:
	          #print line
	          m = re.match('.*KERNEL==\"ion\".*?GROUP=\"users\".*?66.*', line)
	          if m:
	            matched_paths.append(fullpath)
	            matched_lines.append(line)

	  len_found = len(matched_paths)
	  if len_found is 0:
	    self.logger.error("\033[1;33;91m[Error]No ion rules is found in /etc/udev/rules.d, try make&make install in libion\033[0m\n" )
	    return False
	  elif len_found > 1:
	    print("\033[1;33;93m[Warning]Find %d define for ion rules\033[0m\n" % (len_found,))
	    
	    for idx,item in enumerate(matched_paths):

	      self.logger.info("\033[1;33;93m  [%d][%s]\033[0m\n" % (idx, item))
	      self.logger.info("\033[1;33;93m  [%s]\033[0m\n" % (matched_lines[idx],))
	    return True
	  else:
	    self.logger.info("\033[1;33;94m[Succ]Found ion rules in %s, passed\033[0m\n" % (matched_paths[0],))
	    return True

 


	def check_test(self):

		log = subprocess.check_output(['./plugins/ion/test'])
		log=log.strip()

		self.logger.info( log)
		return True

	def check_functions(self,config):
		if not self.check_test():
			return False
		
		return True

    

	def monit(self,config):
		print("ion monit")
		global timer
		timer = threading.Timer(1, check_memory_usage)
		timer.start()

	def cancel_monit(self,configData):
		print("ion cancel_monit")
		global timer
		timer.cancel()
		return True


	def check_env(self,config):

		if not self.check_ion_drv():
			return False

		if not self.check_group():
		  return False

		if not self.check_udev():
		  return False

		return True


def getInstance():
  return ION()
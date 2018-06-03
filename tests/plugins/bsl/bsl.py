#!/usr/bin/env python
# -*- coding: utf-8 -*-
from component import Component
import sys, os, re, string,subprocess,time
import logging


class BSL(Component):
	def __init__(self, name="bsl"):
		super(BSL, self).__init__(name)

	def check_i801_drv(self):
	  log = os.popen('lsmod | grep i2c_i801').read()
	  log=log.strip()
	  if len(log) is 0:
	    self.logger.error("\033[1;33;91m[Error]i2c_i801 is not installed, try sudo modprobe i2c_i801 \033[0m\n" )
	    return False
	  else:
	    self.logger.info("\033[1;33;94m[Succ]i2c_i801 is installed, passed\033[0m\n" )
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
	          m = re.match('.*i2c-\[0-7\].*?GROUP=\"users\".*?66.*', line)
	          if m:
	            matched_paths.append(fullpath)
	            matched_lines.append(line)

	  len_found = len(matched_paths)
	  if len_found is 0:
	    self.logger.error("\033[1;33;91m[Error]No bsl rules is found in /etc/udev/rules.d, try make&make install in libbsl\033[0m\n" )
	    return False
	  elif len_found > 1:
	    self.logger.info("\033[1;33;93m[Warning]Find %d define for bsl rules\033[0m\n" % (len_found,))
	    
	    for idx,item in enumerate(matched_paths):

	      self.logger.info("\033[1;33;93m  [%d][%s]\033[0m\n" % (idx, item))
	      self.logger.info("\033[1;33;93m  [%s]\033[0m\n" % (matched_lines[idx],))
	    return True
	  else:
	    self.logger.info("\033[1;33;94m[Succ]Found bsl rules in %s, passed\033[0m\n" % (matched_paths[0],))
	    return True

	def bootup_all(self,num):
	  for i in range(num):
	    try:
	      log = subprocess.check_output(['../../tools/moviUsbBoot','./plugins/bsl/myriad.mvcmd'])
	      time.sleep(1)
	    except subprocess.CalledProcessError as e:
	      self.logger.error("\033[1;33;93m[Warning]Device bootup may failed\n")
	      time.sleep(1)
	      continue

	def check_reset_all(self,num):

	  log = subprocess.check_output(['bsl_reset'])
	  time.sleep(2)

	  self.bootup_all(num)
	 
	  log = subprocess.check_output(['lsusb','-d','040e:f63b'])
	  log=log.strip()
	  devices = log.split('\n')
	 
	  if(len(devices)!=num):
	  	return False

	  log = subprocess.check_output(['bsl_reset'])
	  time.sleep(2)

	  try:
	    log = subprocess.check_output(['lsusb','-d','040e:f63b'])
	    self.logger.error("\033[1;33;91m[Error]bsl_reset reset all failed\033[0m\n" )
	    return False
	  except subprocess.CalledProcessError as e:
	    #should be here since no device
	    self.logger.info("\033[1;33;94m[Succ]bsl_reset reset all works,passed\033[0m\n")
	    return True
	 

	def check_reset(self,num):

	  log = subprocess.check_output(['bsl_reset'])
	  time.sleep(2)

	  self.bootup_all(num)
	  
	  log = subprocess.check_output(['lsusb','-d','040e:f63b'])
	  log=log.strip()
	  devices = log.split('\n')
	  
	  if(len(devices)!=num):
	  	return False

	  for i in range(num):
	    cmd = "bsl_reset %d" % (i,)
	    #print cmd
	    log = os.popen(cmd).read()
	    #print log
	    time.sleep(1)
	     
	   
	  try:
	    log = subprocess.check_output(['lsusb','-d','040e:f63b'])
	    self.logger.error("\033[1;33;91m[Error]bsl_reset reset failed\033[0m\n" )
	    return False
	  except subprocess.CalledProcessError as e:
	    #should be here since no device
	    self.logger.error("\033[1;33;94m[Succ]bsl_reset reset works,passed\033[0m\n")
	    return True

	 
 
	def check_functions(self,config):
		if not self.check_reset_all(config['dev_num']):
			return False
		if not self.check_reset(config['dev_num']):
			return False
		
		return True

    
	def monit(self,config):
		self.logger.info("bsl monit")
		#raise RuntimeError("BSL not support monit")
		return True

	def cancel_monit(self,configData):
		self.logger.info("bsl cancel_monit")
		return True


	def check_env(self,config):
		if not self.check_i801_drv():
		  return False
		if not self.check_group():
		  return False

		if not self.check_udev():
		  return False

		return True


def getInstance():
  return BSL()
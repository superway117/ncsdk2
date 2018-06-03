#!/usr/bin/env python
# -*- coding: utf-8 -*-
from component import Component
import sys, os, re, string,subprocess,time
import json
import threading
import logging
from collections import deque

timer = None 
module_logger = logging.getLogger('ncsdk')

def check_usb():
	global module_logger
	try:
		log = subprocess.check_output(['lsusb','-d','040e:f63b'])
		log=log.strip()
		devices = log.split('\n')
		module_logger.info("[Info]bootup device num %d" % (len(devices),)) 
	except subprocess.CalledProcessError as e:
		#should be here since no device
		module_logger.info("[Info]bootup device num 0")
	finally:
		global timer
		timer = threading.Timer(5.5, check_usb)
		timer.start()

class NCSDK(Component):
	def __init__(self, name="ncsdk"):
		super(NCSDK, self).__init__(name)


	def check_vsc_drv(self):
	  log = os.popen('lsmod | grep myd_vsc').read()
	  log=log.strip()
	  if len(log) is 0:
	    self.logger.error("\033[1;33;91m[Error]myd_vsc is not installed, try sudo insmod myd_vsc.ko \033[0m\n" )
	    return False
	  else:
	    self.logger.info("\033[1;33;94m[Succ]myd_vsc is installed, passed\033[0m\n" )
	    return True

	def check_vsc_rule(self):
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
	          m = re.match('.*?SUBSYSTEM==\"usb\".*?ATTRS\{idProduct\}==\"2485\".*?GROUP=\"users\".*?66.*', line)
	          if m:
	            matched_paths.append(fullpath)
	            matched_lines.append(line)

	  len_found = len(matched_paths)
	  if len_found is 0:
	    self.logger.error("\033[1;33;91m[Error]No ncsdk usb rules is found in /etc/udev/rules.d, try make&make install in libion\033[0m\n" )
	    return False
	  elif len_found > 1:
	    self.logger.info("\033[1;33;93m[Warning]Find %d define for ncsdk usb rules\033[0m\n" % (len_found,))
	    
	    for idx,item in enumerate(matched_paths):

	      self.logger.info("\033[1;33;93m  [%d][%s]\033[0m\n" % (idx, item))
	      self.logger.info("\033[1;33;93m  [%s]\033[0m\n" % (matched_lines[idx],))
	    return True
	  else:
	    self.logger.info("\033[1;33;94m[Succ]Found ncsdk usb rules in %s, passed\033[0m\n" % (matched_paths[0],))
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


	def check_tty_rule(self):
	  rules_dir = '/etc/udev/rules.d'
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
	          m = re.match('.*?SUBSYSTEM==\"tty\".*?ATTRS\{idProduct\}==\"2485\".*?GROUP=\"users\".*?66.*', line)
	          if m:
	            matched_paths.append(fullpath)
	            matched_lines.append(line)

	  len_found = len(matched_paths)
	  if len_found is 0:
	    self.logger.error("\033[1;33;91m[Error]No ncsdk tty rule is found in /etc/udev/rules.d, try make&make install in libion\033[0m\n" )
	    return False
	  elif len_found > 1:
	    self.logger.info("\033[1;33;93m[Warning]Find %d define for ncsdk tty rule\033[0m\n" % (len_found,))
	    
	    for idx,item in enumerate(matched_paths):

	      self.logger.info("\033[1;33;93m  [%d][%s]\033[0m\n" % (idx, item))
	      self.logger.info("\033[1;33;93m  [%s]\033[0m\n" % (matched_lines[idx],))
	    return True
	  else:
	    self.logger.info("\033[1;33;94m[Succ]Found ncsdk tty rules in %s, passed\033[0m\n" % (matched_paths[0],))
	    return True

 
	def loadJsonFromFile(self,jsonPath):
		with open(jsonPath, 'rt') as f:
			try:
			
			  data = f.read()
			  jsonObj = json.loads(data)
			  return jsonObj
			except IOError as e:
			  self.logger.error(  e )
			  sys.exit()
			except Exception as e:
				self.logger.error("\033[1;33;91m[Error]Some error/exception occurred when load json file:%s\033[0m\n" % (jsonPath,))
				self.logger.error("\033[1;33;91m[Error]Err detail:\n%s\033[0m\n" % (e,))
				sys.exit()

	def check_graph(self):
		jsonObj =  self.loadJsonFromFile("./plugins/ncsdk_204/config/icv_async.json")
		for item in jsonObj:
			if not os.path.isfile(item['graph_paths'][0]):
				self.logger.error("\033[1;33;91m[Error]graph path is not exist\033[0m\n" )
				return False
			if not os.path.isfile(item['tensor']):
				self.logger.error("\033[1;33;91m[Error]tensor path is not exist\033[0m\n" )
				return False

			log = subprocess.check_output([
				'python',
				'../../tools/grahp_dump/main.py',
				'-g',
				item['graph_path']
			])
			log=log.strip()
			lines = log.split("\n")
			last_line = lines[-1]
 
			self.logger.info( log)
 
			if cmp(last_line,"Main Func Exit Successfully!")==0:
				self.logger.info("\033[1;33;94m[Succ]Graph file format is ok\033[0m\n" )
				return True
			else:
				self.logger.error("\033[1;33;91m[Error]Looks graph file format is not right\033[0m\n" )
				return False
 

	def check_async(self):
		self.logger.info("\033[1;32;40mstart icv_async_ion....\nafter finished,you can check log in screen or log.txt\033[0m")
		log = os.popen('./plugins/ncsdk_204/src/icv_async_ion').read()
		log=log.strip()

		self.logger.info( log)
		log=log.strip()
		lines = log.split("\n")
		last_line = lines[-1]

		if cmp(last_line,"Main Func Exit Successfully!")==0:
			self.logger.info("\033[1;33;94m[Succ]icv_async_ion passed\033[0m\n" )
			return True
		else:
			self.logger.error("\033[1;33;91m[Error]icv_async_ion failed\033[0m\n" )
			self.logger.error("\033[1;33;91m[Error]try  ./plugins/ncsdk_204/src/icv_async_ion\033[0m\n" )
			return False

	def check_graph(self):
		self.logger.info("\033[1;32;40mstart icv_graph_ion....\nafter finished,you can check log in screen or log.txt\033[0m")
		log = os.popen('./plugins/ncsdk_204/src/icv_graph_ion').read()
		log=log.strip()

		self.logger.info( log)
		log=log.strip()
		lines = log.split("\n")
		last_line = lines[-1]

		if cmp(last_line,"Main Func Exit Successfully!")==0:
			self.logger.info("\033[1;33;94m[Succ]icv_graph_ion passed\033[0m\n" )
			return True
		else:
			self.logger.error("\033[1;33;91m[Error]icv_graph_ion failed\033[0m\n" )
			self.logger.error("\033[1;33;91m[Error]try ./plugins/ncsdk_204/src/icv_sync_ion\033[0m\n" )
			return False
	 


	def check_sync(self):
		self.logger.info("\033[1;32;40mstart icv_sync_ion....\nafter finished,you can check log in screen or log.txt\033[0m")
		log = os.popen('./plugins/ncsdk_204/src/icv_sync_ion').read()
		log=log.strip()

		self.logger.info( log)
		log=log.strip()
		lines = log.split("\n")
		last_line = lines[-1]

		if cmp(last_line,"Main Func Exit Successfully!")==0:
			self.logger.info("\033[1;33;94m[Succ]icv_sync_ion passed\033[0m\n" )
			return True
		else:
			self.logger.error("\033[1;33;91m[Error]icv_sync_ion failed\033[0m\n" )
			self.logger.error("\033[1;33;91m[Error]try ./plugins/ncsdk_204/src/icv_sync_ion\033[0m\n" )
			return False


	def check_no_ion(self):
		self.logger.info("\033[1;32;40mstart icv_async_noion....\nafter finished,you can check log in screen or log.txt\033[0m")
		log = os.popen('./plugins/ncsdk_204/src/icv_async_noion').read()
		log=log.strip()

		self.logger.info( log)
		log=log.strip()
		lines = log.split("\n")
		last_line = lines[-1]

		if cmp(last_line,"Main Func Exit Successfully!")==0:
			self.logger.info("\033[1;33;94m[Succ]icv_async_noion passed\033[0m\n" )
			return True
		else:
			self.logger.error("\033[1;33;91m[Error]icv_async_noion failed\033[0m\n" )
			self.logger.error("\033[1;33;91m[Error]try ./plugins/ncsdk_204/src/icv_async_noion\033[0m\n" )
			return False


	def check_functions(self,config):
		if not self.check_graph():
			return False
		if not self.check_no_ion():
			return False
		if not self.check_sync():
			return False
		if not self.check_async():
			return False

		
		return True

    

	def monit(self,config):
		global timer
		timer = threading.Timer(1, check_usb)
		timer.start()

	def cancel_monit(self,configData):
		global timer
		timer.cancel()
		return True


	def check_env(self,config):

		if not self.check_vsc_drv():
			return False

		if not self.check_vsc_rule():
			return False

		if not self.check_tty_rule():
		  return False

		if not self.check_group():
		  return False

		

		return True


def getInstance():
  return NCSDK()

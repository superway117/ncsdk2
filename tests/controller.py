# -*- coding: utf-8 -*-
import sys, os, re, string,time
import json

import threading
from datetime import datetime
import imp

from colorama import init
from colorama import Fore, Back, Style

from component import Component
from log import create_logger


class Controller(object):
  def __init__(self,configPath="config.json"):
    #init() # for color print
    self.config_path = configPath
    self.component_name_list = []
    self.component_list = []
    self.logger = create_logger("Controller")
    self.loadConfig()
    self.loadComponents()
 

  def _dynLoadModule(self,name, globals=None, locals=None, fromlist=None):
    try:
       return sys.modules[name]
    except KeyError:
       pass
    plugin_path = "./plugins/%s" % (name,)
    fp, pathname, description = imp.find_module(name,[plugin_path])

    try:
      return imp.load_module(name, fp, pathname, description)
    finally:
      # Since we may exit via an exception, close fp explicitly.
      if fp:
        fp.close()

  def loadJsonFromFile(self,jsonPath):
    with open(jsonPath, 'rt') as f:
      try:
      
        data = f.read()
        jsonObj = json.loads(data)
        self.logger.info(  "jsonObj:\n\t%s" % (jsonObj,) )
        return jsonObj
      except IOError as e:
        self.logger.error(  e )
        sys.exit()
      except Exception as e:
        self.logger.error( "\nSome error/exception occurred when load json file:%s" % (jsonPath,))
        self.logger.error( "Err detail:\n%s" % (e,))
        sys.exit()

 
  def loadConfig(self):
    self.configData = self.loadJsonFromFile(self.config_path)

    if "components" not in self.configData:
      self.logger.error("no components found in config file")
      raise RuntimeError('components must be included in config file!')
    
    self.component_name_list = self.configData['components']

  def loadComponents(self):
    for component_name in self.component_name_list:
      component = self._dynLoadModule(component_name)
      instance = component.getInstance()
      self.component_list.append(instance)

  def check_functions(self):
    results = []
    for idx,component in enumerate(self.component_list):
      self.logger.info("\033[1;32;40mstart checking function of %-10s\033[0m" % (self.component_name_list[idx],))
      ret = component.check_functions(self.configData)
      results.append(ret)
    self.logger.info("\n--------------Check Functions Summary--------------------")
    self.logger.info("\033[1;32;40m%-10s%-10s%-10s\033[0m" % ("Index","Name","Passed"))
    for idx, item in enumerate(results):
      self.logger.info("%-10d%-10s%-10s" % (idx,self.component_name_list[idx],item))




  def monit(self):
    for idx,component in enumerate(self.component_list):
      component.monit(self.configData)

  def cancel_monit(self):
    for idx,component in enumerate(self.component_list):
      component.cancel_monit(self.configData)

  def check_env(self):
    results = []
    for idx,component in enumerate(self.component_list):
      self.logger.info("\033[1;32;40mstart checking env of %-10s\033[0m" % (self.component_name_list[idx],))
      ret = component.check_env(self.configData)
      results.append(ret)
    self.logger.info("\n--------------Check Env Summary--------------------")
    self.logger.info("\033[1;32;40m%-10s%-10s%-10s\033[0m" % ("Index","Name","Passed"))
    for idx, item in enumerate(results):
      self.logger.info("%-10d%-10s%-10s" % (idx,self.component_name_list[idx],item))

  def check(self):
    env_results = []
    function_results = []
    for idx,component in enumerate(self.component_list):
      self.logger.info("\033[1;32;40mstart checking env of %-10s\033[0m" % (self.component_name_list[idx],))
      ret = component.check_env(self.configData)
      env_results.append(ret)
      self.logger.info("\033[1;32;40mstart checking function of %-10s\033[0m" % (self.component_name_list[idx],))
      ret = component.check_functions(self.configData)
      function_results.append(ret)

    self.logger.info("\n--------------Check Env Summary--------------------")
    self.logger.info("\033[1;32;40m%-10s%-10s%-10s\033[0m" % ("Index","Name","Passed"))
    for idx, item in enumerate(env_results):
      self.logger.info("%-10d%-10s%-10s" % (idx,self.component_name_list[idx],item))

    self.logger.info("\n--------------Check Functions Summary--------------------")
    self.logger.info("\033[1;32;40m%-10s%-10s%-10s\033[0m" % ("Index","Name","Passed"))
    for idx, item in enumerate(function_results):
      self.logger.info("%-10d%-10s%-10s" % (idx,self.component_name_list[idx],item))


  def debug(self):
    for component in self.component_list:
      component.debug(self.configData)
 

  def __iter__(self):
    return iter(self.component_list)


if __name__ == '__main__':

  controller = Controller()


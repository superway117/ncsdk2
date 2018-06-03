#!/usr/bin/env python
# -*- coding: utf-8 -*-
from log import create_logger
def timethis(func):
  '''
  Decorator that reports the execution time.
  '''
  @wraps(func)
  def wrapper(*args, **kwargs):
    start = time.time()
    result = func(*args, **kwargs)
    end = time.time()
    print(func.__name__, end-start)
    return result
  return wrapper

class Component(object):
  def __init__(self, name):
    self._name = name.strip()
    self.logger = create_logger(self._name)
		
  @property
  def name(self):
    return self._name

  @name.setter
  def name(self,name):
    self._name = name
 
  def check_functions(self,configData):
    return None
    
  def monit(self,configData):
  	return None

  def cancel_monit(self,configData):
    return None

  def check_env(self,configData):
    return None

  def debug(self,configData):
    return None
# -*- coding: utf-8 -*-

from functools import wraps, partial
import logging

import functools
import logging
 
def create_logger(name="myx_debugger"):
  """
  Creates a logging object and returns it
  """
  logger = logging.getLogger(name)
  #logger.setLevel(logging.INFO)
  logger.setLevel(logging.DEBUG)
  # create the logging file handler
  fh = logging.FileHandler("./log.txt")
 
  #fmt = '[%(name)s]%(message)s'
  #formatter = logging.Formatter(fmt)
  #fh.setFormatter(formatter)
  fh.setLevel(logging.DEBUG)

  # add handler to logger object
  logger.addHandler(fh)

  #fmt = '[%(name)s]%(message)s'
  #formatter = logging.Formatter(fmt)
  

  ch = logging.StreamHandler()
  ch.setLevel(logging.DEBUG)
  #ch.setFormatter(formatter)
  logger.addHandler(ch)
  return logger
 

def logged(func=None, level=logging.DEBUG, name=None, message=None):

  if func is None:
    return partial(logged, level=level, name=name, message=message)

  logname = name if name else func.__module__

  log = logging.getLogger(logname)
  logmsg = message if message else func.__name__

  @wraps(func)
  def wrapper(*args, **kwargs):
    log.log(level, logmsg)
    return func(*args, **kwargs)

  return wrapper

__all__ = ['create_logger','logged']

#!/usr/bin/env python
# -*- coding: utf-8 -*-
import time
from functools import wraps
from log import create_logger

#logger = create_logger("Util")
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

__all__ = ["timethis"]
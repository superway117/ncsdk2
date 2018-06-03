#!/usr/bin/python
# written by Eason
# Mail eason.pan@intel.com


import getopt,os
from ctypes import c_uint
from GraphParser import GraphParser
import sys
import traceback

def usage():
  print "python main.py -g graph_path"

 
if __name__ == '__main__':

  try:
    opts, args = getopt.getopt(sys.argv[1:], "g:", ["graph="])

  except getopt.GetoptError as err:
    print str(err) # will print something like "option -a not recognized"
    usage()
    sys.exit(1)

  #graph_path = "SYS_SW.elf"
  hashChangeList={}

  for o, a in opts:
    if o == "-g":
      graph_path = a
    else:
      print "unhandled option"


  if not os.path.isfile(graph_path):
    print "invalid graph path: %s" % (graph_path,)
    sys.exit(1)


  print "graph path is: %s" % (graph_path,)

  try:
    elf = GraphParser(graph_path,onlyParseHeader=True)
    elf.print_graph()
    print "passed"
  except Exception as e:
    traceback.print_exc()
    print "failed"
 
  
  







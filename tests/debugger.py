# -*- coding: utf-8 -*-
import getopt, sys
from controller import Controller 

def usage():
	print("python main.py -a [all|env|function|debug] -c config.json")
	print("python main.py --action=[all|env|function|debug] --config=config.json")

 
if __name__ == '__main__':

	try:
		opts, args = getopt.getopt(sys.argv[1:], "ha:c:p:", ["help", "action=","config=","plugin="])

	except getopt.GetoptError as err:
		print (err) # will print something like "option -a not recognized"
		usage()
		sys.exit(2)
 
	action = "run"
	config = "config.json"
	 
	plugin = None
	
	for o, a in opts:
		if o == "-a":
			action = a
		elif o in ("-h", "--help"):
			usage()
			sys.exit()
		elif o in ("-c", "--config"):
			config = a
		elif o in ("-p", "--plugin"):
			plugin = a
		else:
		  assert False, "unhandled option"

	controller = Controller(config)
	if action == "all":
		controller.check()
	elif action == "env":
		controller.check_env()
	elif action == "function":
		controller.check_functions()
	elif action == "monit":
		controller.monit()
	elif action == "debug":
		controller.debug()
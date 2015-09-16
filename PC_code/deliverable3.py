# Python 2.7 code to load binary files on to the Stellaris
# using "LM Flash Programmer". This code assumes that the
# programmer is located in "C:\LMFlash.exe". You can 
# download it from the TI website.
#
# Author: Martin Steinke 2015.



# import the class that interfaces with the programmer.
from load_test import load_test 
import os # For navigating the system.

# Make an object to represent the Stellaris.
ESTR = load_test()

# Display some instructions
print "-------------------------------------------------------------\n"
print "Welcome to Group 06's basic UUT test manger!\n"
print "Enter 1 for UART Mirror test, 2 for GPIO test or q to quit.\n"
print "-------------------------------------------------------------\n"

# Loop around; asking for input from the user.
while True:
	# Get user input.
	test_num = raw_input("Enter test number: ")
	
	# Check input to see if user wants to quit.
	if test_num == "q": 
		break
	# Check if input is valid.
	elif test_num not in ["1", "2", "q"]:
		print "Invalid Entry!"
	# Load the test onto Stellaris if input is valid.
	else:
		_path = os.getcwd() + "/testBins/test{}.bin".format(test_num)
		ESTR.loadTest(_path)
	print "\n"

# Done	
print "That's all folks!"



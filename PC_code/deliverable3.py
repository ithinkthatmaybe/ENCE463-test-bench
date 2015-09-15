from load_test import load_test
import os

testThing = load_test()

#testThing.loadTest("P:/ENCE463/testBins/test1.bin")
print "-------------------------------------------------------------\n"
print "Welcome to Group 06's basic UUT test manger!\n"
print "Enter 1 for UART Mirror test, 2 for GPIO test or q to quit.\n"
print "-------------------------------------------------------------\n"

while True:
	test_num = raw_input("Enter test number: ")
	if test_num == "q":
		break
	elif test_num not in ["1", "2", "q"]:
		print "Invalid Entry!"
	else:
		_path = os.getcwd() + "/testBins/test{}.bin".format(test_num)
		testThing.loadTest(_path)
	print "\n"
print "That's all folks!"
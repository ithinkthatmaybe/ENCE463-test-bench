# @created 25-8-2015
# @author MCS
# @description load_test.py module for programming the ESTR with new tests.

#import telnetlib # for interfacing with openOCD.
import os # For navigating the system.

class Load_test(object):

    def __init__(self): # start openOCD, connect to it and configure it.
		"""Initialise the ESTR object"""
		self.lmflashDirectory = "C:\LMFlash.exe" # programmer location
		self.lmflashQuickSet = "ek-lm3s1968" # device name
		
    def loadTest(self, file): # program a new test on to the ESTR.
		"""start 'LM Flash Programmer', load code on to ESTR.
		   The -q option is for quickset; setting which device to use.
		   The -v option is to verify after flashing.
		   The -r option is to reset after flashing. """
		string = self.lmflashDirectory + " -q " + self.lmflashQuickSet + " -v -r " + file
		os.system(string) # instigate command.
		
    def resetESTR(self): # reboot the ESTR.
		"""Performs a hardware reset on the connected target device."""
		string = self.lmflashDirectory + " --hreset"
		os.system(string) # instigate command.
    #def startESTR(self) # start the ESTR if it has been stopped or paused.
    #def stopESTR(self) # stops the ESTR from running.
    #gdef getDeviceStatus(self) # return the status of the ESTR.
    
    



    

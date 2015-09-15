# @created 25-8-2015
# @author MCS
# @description load_test.py module for programming the ESTR with new tests.

import telnetlib # for interfacing with openOCD.
import os

class load_test(object):

	
	
    def __init__(self): # start openOCD, connect to it and configure it.
		"""start openOCD, connect to it and configure it."""
		self.lmflashDirectory = "C:\LMFlash.exe"
		self.lmflashQuickSet = "ek-lm3s1968"
		#self.lmflashfile = "audio.bin"
		#os.system("START " + self.lmflashDirectory + " -q " + self.lmflashQuickSet + " -v -r " + self.lmflashfile)
    def loadTest(self, file): # program a new test on to the ESTR.
		string = self.lmflashDirectory + " -q " + self.lmflashQuickSet + " -v -r " + file
		print string
		os.system(string)
    #def resetESTR(self) # reboot the ESTR.
    #def startESTR(self) # start the ESTR if it has been stopped or paused.
    #def stopESTR(self) # stops the ESTR from running.
    #def getOCDStatus(self) # return the status of openOCD.
    #gdef getDeviceStatus(self) # return the status of the ESTR.
    
    



    

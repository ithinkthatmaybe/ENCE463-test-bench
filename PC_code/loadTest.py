# @created 25-8-2015
# @author MCS
# @description load_test.py module for programming the ESTR with new tests.

#import telnetlib # for interfacing with openOCD.
import os # For navigating the system.

class LoadTest(object):
    UART = 1
    GPIO = 2
    BLANK = 3
    currentTest = UART # current state of the ESTR
    STARTCHAR = '$'
    waitingForTest = False
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

    def interpretUARTSubTests(self, resultString):
        """Determines if the result is a pass or fail for each subtest of.
        Reference is to section 2.1.2 Tests."""
        subtest_number = resultString[0]
        message = ""
        
        ##############################################################
        # section 2.1.2a) Mirror message TX to UUT:
        if subtest_number == "1": # section 2.1.2a
            if "Fail" in resultString:
                message =  "UART sub-test {} failed.".format(subtest_number)
            elif "Pass" in resultString:
                message =  "UART sub-test {} passed.".format(subtest_number)
            else:
                message =  "UART sub-test {} result received but not interpreted.".format(subtest_number)
        
        ##############################################################
        # section 2.1.2b) Mirror message RX by ESTR:
        elif subtest_number == "2":
            airspeed = ""
            t = "" #internal CPU temperature (ADC DTC)
            v = "" #internal voltage measurement (ADC DTC)
            alt = "" #altitude
            newString = resultString.replace(" ", "") # remove white space
            if newString[0:2] == "ST": # check that it is in status mesasge format
                pass


        ##############################################################
        # section 2.1.2c) Message Check
        elif subtest_number == "3":
            message =  "UART sub-test {} result received but not interpreted.".format(subtest_number)
        
        ##############################################################
        # section 2.1.2d) Test for the effect of UUT processor clock variation.
        elif subtest_number == "4":
            message =  "UART sub-test {} result received but not interpreted.".format(subtest_number)
        
        ##############################################################
        # in case subtest number is not recognised.
        else:
            message = "Invalid UART subtest number received."
        return message

    def interpretTest(self, resultString):
        """interprets test results. This assumes that the start character, 
        $, has been removed from resultString string."""
        
        #subtest_number = resultString[0]
        message = ""

        if LoadTest.currentTest == LoadTest.UART:
            message = self.interpretUARTSubTests(resultString)
        elif LoadTest.currentTest == LoadTest.GPIO:
            pass
        elif LoadTest.currentTest == LoadTest.BLANK:
            pass
        else:
            message = "Error. Could not interpret Test."
        return "\n>> " + message

    

    #def startESTR(self) # start the ESTR if it has been stopped or paused.
    #def stopESTR(self) # stops the ESTR from running.
    #gdef getDeviceStatus(self) # return the status of the ESTR.

# @created 25-8-2015
# @author MCS
# @description load_test.py module for programming the ESTR with new tests.

#import telnetlib # for interfacing with openOCD.
import os # For navigating the system.


class LoadTest(object):
    
    def __init__(self): # start openOCD, connect to it and configure it.
        """Initialise the ESTR object"""
        self.lmflashDirectory = "C:\LMFlash.exe" # programmer location
        self.lmflashQuickSet = "ek-lm3s1968" # device name

        #define/enum some tests
        self.UART = 1
        self.GPIO = 2
        self.BLANK = 3
        self.currentTest = self.BLANK # current state of the ESTR
        self.STARTCHAR = '$'
        self.waitingForTest = False

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
        newString = resultString[2:-1]
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
            
            airspeed = newString[6:12]
            t = newString[15:20] #internal CPU temperature (ADC DTC)
            v = newString[23:29] #internal voltage measurement (ADC DTC)
            alt = newString[33:] #altitude
            #newString = resultString.replace(" ", "")[2:-1] # remove white space
            

            if newString[0:2] == "ST": # check that it is in status mesasge format
                if len(newString) == 38:
                    message =  "UART sub-test {} passed.".format(subtest_number)
                else:
                    message =  "UART sub-test {} failed. Length = {}.".format(subtest_number, len(newString))
                message += "\n>>     airspeed = {}".format(airspeed)
                message += "\n>>     temperature = {}".format(t)
                message += "\n>>     voltage = {}".format(v)
                message += "\n>>     altitude = {}".format(alt)
            else:
                message =  "UART sub-test {} failed. 'ST' identifier not present.".format(subtest_number)
            #message = "{}".format(infoList)
            



        ##############################################################
        # section 2.1.2c) Message Check
        elif subtest_number == "3":
            message =  "UART sub-test {} result received but not interpreted.".format(subtest_number)
                
            
        
        ##############################################################
        # section 2.1.2d) Test for the effect of UUT processor clock variation.
        elif subtest_number == "4":
            tempString = newString.split(',')
            inc = tempString[0]
            dec = tempString[1]
            message =  "UART sub-test {}:".format(subtest_number)
            message += "\n>>     increment = {}".format(inc)
            message += "\n>>     decrement = {}".format(dec)
        
        ##############################################################
        # in case subtest number is not recognised.
        else:
            message = "Invalid UART subtest number ({}) received.".format(subtest_number)
        return message

    def interpretGPIOSubTests(self, resultString):
        """Determines if the result is a pass or a fail."""
        message = ""
        subtest_number = resultString[0]
        newString = resultString[2:-1]
        ##############################################################
        # section 2
        if subtest_number == "1":
            
            num_zeros = newString[2:-1].count("0")
            num_ones = newString[2:-1].count("1")
            if num_zeros > 0:
                result = "failed"
            else:
                result = "passed"
            message =  "GPIO sub-test {} air speed response {} ([zeros:ones] = [{}:{}]).".format(subtest_number, result, num_zeros, num_ones)
            #
            splitString = newString.split(',')
            average = sum(splitString[:-1])/len(splitString)
            minimum = min(splitString[:-1])
            maximum = max(splitString[:-1])
            mode = mode(splitString[:-1])
            message += "\n>>     average = {}".format(average)
            message += "\n>>     minimum = {}".format(minimum)
            message += "\n>>     maximum = {}".format(maximum)
            message += "\n>>     mode = {}".format(mode)

        ##############################################################
        # section 2
        elif subtest_number == "2":
            message =  "GPIO sub-test {}.".format(subtest_number)
        ##############################################################
        # in case subtest number is not recognised.
        else:
            message =  "Invalid UART subtest number ({}) received.".format(subtest_number)
        return message

    def interpretTest(self, resultString):
        """interprets test results. This assumes that the start character, 
        $, has been removed from resultString string."""
        
        #subtest_number = resultString[0]
        message = ""

        if self.currentTest == self.UART:
            message = self.interpretUARTSubTests(resultString)
        elif self.currentTest == self.GPIO:
            message = self.interpretGPIOSubTests(resultString)
        elif self.currentTest == self.BLANK:
            message = "BLANK Test."
        else:
            message = "Error. Could not interpret Test."
        return "\n>> " + message #+ "{}".format(self.currentTest)

    

    #def startESTR(self) # start the ESTR if it has been stopped or paused.
    #def stopESTR(self) # stops the ESTR from running.
    #gdef getDeviceStatus(self) # return the status of the ESTR.

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
        self.COMBINED = 4
        self.currentTest = self.COMBINED # current state of the ESTR
        self.STARTCHAR = '$'
        self.ENDCHAR = '\r'
        self.waitingForTestBool = False
        self.packetCount = "0" # counts the number of packets a test has recieved.
        self.packet1 = ""# assemble packets here.
        self.packet2 = ""# assemble packets here.
        self.packet3 = ""# assemble packets here.
        self.packet4 = ""# assemble packets here.
        self.i = 0
        self.GPIO5THRESHOLD = 1000 # us
        #self.allTest = False
        #self.allTestnum = 0

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

    def testErrorCodes(self, data):
        """interprets the error codes that the ESTR may send."""
        message = "UART error code detected ({}).".format(data)
        return "\n>>" + message

    def testUART0(self, data):
        """Determines if the result is a pass or fail for each subtest of.
        Reference is to section 2.1.2 Tests."""
        subtest_number = "0"
        message = ""

        if "Fail" in data:
            message =  "UART sub-test {} failed.".format(subtest_number)
        elif "Pass" in data:
            message =  "UART sub-test {} passed.".format(subtest_number)
        else:
            message =  "UART sub-test {} result received but not interpreted.".format(subtest_number)
        return "\n>>" + message

    def testUART1(self, data):
        """Determines if the result is a pass or fail for each subtest of.
        Reference is to section 2.1.2 Tests."""
        subtest_number = "1"
        message = ""
        message =  "UART sub-test {} result received but not interpreted.".format(subtest_number)

        return "\n>>" + message

    def testUART2(self, data):
        """Determines if the result is a pass or fail for each subtest of.
        Reference is to section 2.1.2 Tests."""
        subtest_number = "2"
        message = ""
        airspeed = data[6:12]
        t = data[15:20] #internal CPU temperature (ADC DTC)
        v = data[23:29] #internal voltage measurement (ADC DTC)
        alt = data[33:] #altitude
        #data = resultString.replace(" ", "")[2:-1] # remove white space
        
        if data[0:2] == "ST": # check that it is in status mesasge format
            if len(data) == 38:
                result = "passed"
            else:
                result = "failed"
            message =  "UART sub-test {} {}. Length = {}.".format(subtest_number, result, len(data))
            message += "\n>>     airspeed = {} m/s".format(airspeed)
            message += "\n>>     temperature = {} C".format(t)
            message += "\n>>     voltage = {} V".format(v)
            message += "\n>>     altitude = {} m".format(alt)
        else:
            message =  "UART sub-test {} failed. 'ST' identifier not present.".format(subtest_number)
        #message = "{}".format(infoList)

        return "\n>>" + message

    def testUART3(self, data):
        """Determines if the result is a pass or fail for each subtest of.
        Reference is to section 2.1.2 Tests."""
        subtest_number = "3"
        message = ""

        expectedResponse = "ERR:6ERR:6ERR:6ERR:6ERR:6ERR:6ERR:6ERR:6ERR:6ERR:6ERR:6ERR:6"
        if data == expectedResponse:
            result = "passed"
        else:
            result = "failed"
        message =  "UART sub-test {} emergency mode test {}. ".format(subtest_number, result)


        #message =  "UART sub-test {} result received but not interpreted.".format(subtest_number)

        return "\n>>" + message

    def testUART4(self, data):
        """Determines if the result is a pass or fail for each subtest of.
        Reference is to section 2.1.2 Tests. check for Clock variation."""
        subtest_number = "4"
        message = ""

        tempString = data.split(',')
        inc = tempString[0]
        dec = tempString[1]
        message =  "UART sub-test {} Clock variation:".format(subtest_number)
        message += "\n>>     UUT clock speed incremented {} times before failer occured.".format(inc)
        message += "\n>>     UUT clock speed decremented {} times before failer occured.".format(dec)

        return "\n>>" + message

    def testGPIO5(self, data):
        """Determines if the result is a pass or fail for each subtest of.
        Reference is to section 2.2.2 Tests."""
        subtest_number = "5"
        message = "bleh "
        #splitString = ""

        #message = self.packet1
        #if self.packetCount == "0":

        # if len(self.packet1) == 0:
        if self.i == 0:
            message = self.packet1
            self.packet1 = data
            self.i += 1;
            
            #self.packetCount = "1"
            #message = "packet is one "
        elif self.i == 1:
        #if self.packetCount == "1":
            self.packet2 = data
            self.i += 1;

            data1 = self.packet1.split(',')
            data2 = self.packet2.split(',')
            #self.packets = ""
            

            as_response_flags = data1 #splitString[:len(splitString)/2 + 1]
            as_latency = data2 #splitString[len(splitString)/2:]

            num_zeros = as_response_flags.count("0") 
            num_ones = as_response_flags.count("1") 
            num_twos = as_response_flags.count("2") + as_response_flags.count("3")
            # iterate through, find mean, max, threshold
            as_latencyTotal = 0
            maximum = 0
            minimum = 1000
            badFlag = False
            for as_latencyTime in as_latency:
                as_latencyTotal += int(as_latencyTime)
                if int(as_latencyTime) >= self.GPIO5THRESHOLD:
                    badFlag = True
                if int(as_latencyTime) > maximum:
                    maximum = int(as_latencyTime)
                if int(as_latencyTime) < minimum:
                    minimum = int(as_latencyTime)

            #splitString = self.packets.split(',')

            if num_zeros > 0 or badFlag == True or num_twos > 1:
                result = "failed"
            else:
                result = "passed"
            message =  "GPIO sub-test {} air speed response {}. ".format(subtest_number, result)
            #
            message += "\n>>     non-responses = {}.".format(num_zeros)
            message += "\n>>     responses = {}.".format(num_ones)
            message += "\n>>     double and triple responses = {}.".format(num_twos)
            as_latencyTotal = 0
            for as_latencyTime in as_latency:
                as_latencyTotal += int(as_latencyTime)

            message += "\n>>     average airspeed latency = {} us".format(as_latencyTotal/len(as_latency))
            message += "\n>>     maximum airspeed latency = {} us".format(maximum)
            message += "\n>>     minimum airspeed latency = {} us".format(minimum)

            self.packet1 = ""
            self.packet2 = ""
            self.i = 0

            return "\n>>" + message# + "{}".format(self.packetCount)

        return ''

    def testGPIO6(self, data):
        """Determines if the result is a pass or fail for each subtest of.
        Reference is to section 2.2.2 Tests."""
        subtest_number = "6"
        message = ""
        if self.i == 0:
            message = self.packet1
            self.packet1 = data
            self.i += 1;
            
            #self.packetCount = "1"
            #message = "packet is one "
        elif self.i == 1:
        #if self.packetCount == "1":
            self.packet2 = data
            self.i += 1;

            data1 = self.packet1.split(',')
            data2 = self.packet2.split(',')
            #self.packets = ""
            

            as_response_flags = data1 #splitString[:len(splitString)/2 + 1]
            as_latency = data2 #splitString[len(splitString)/2:]

            num_zeros = as_response_flags.count("0") 
            num_ones = as_response_flags.count("1") 
            num_twos = as_response_flags.count("2") + as_response_flags.count("3")
            # iterate through, find mean, max, threshold
            as_latencyTotal = 0
            maximum = 0
            minimum = 1000
            badFlag = False
            for as_latencyTime in as_latency:
                as_latencyTotal += int(as_latencyTime)
                if int(as_latencyTime) >= self.GPIO5THRESHOLD:
                    badFlag = True
                if int(as_latencyTime) > maximum:
                    maximum = int(as_latencyTime)
                if int(as_latencyTime) < minimum:
                    minimum = int(as_latencyTime)

            #splitString = self.packets.split(',')

            if num_zeros > 0 or badFlag == True or num_twos > 1:
                result = "failed"
            else:
                result = "passed"
            message =  "GPIO sub-test {} air speed response {}. ".format(subtest_number, result)
            #
            message += "\n>>     non-responses = {}.".format(num_zeros)
            message += "\n>>     responses = {}.".format(num_ones)
            message += "\n>>     double and triple responses = {}.".format(num_twos)
            as_latencyTotal = 0
            for as_latencyTime in as_latency:
                as_latencyTotal += int(as_latencyTime)

            message += "\n>>     average airspeed latency = {} us".format(as_latencyTotal/len(as_latency))
            message += "\n>>     maximum airspeed latency = {} us".format(maximum)
            message += "\n>>     minimum airspeed latency = {} us".format(minimum)

            self.packet1 = ""
            self.packet2 = ""
            self.i = 0

            return "\n>>" + message# + "{}".format(self.packetCount)

        return ''

    def testGPIO7(self, data):
        """Determines if the result is a pass or fail for each subtest of.
        Reference is to section 2.2.2 Tests."""
        subtest_number = "7"
        message = ""
        if self.i == 0:
            message = self.packet1
            self.packet1 = data
            self.i += 1;

        elif self.i == 1:
            message = self.packet2
            self.packet2 = data
            self.i += 1;

            data1 = self.packet1.split(',')
            data2 = self.packet2.split(',')
            #self.packets = ""
            

            as_response_flags = data1 #splitString[:len(splitString)/2 + 1]
            as_latency = data2 #splitString[len(splitString)/2:]

            num_zeros = as_response_flags.count("0") 
            num_ones = as_response_flags.count("1") 
            num_twos = as_response_flags.count("2") + as_response_flags.count("3")
            # iterate through, find mean, max, threshold
            as_latencyTotal = 0
            maximum = 0
            minimum = 1000
            badFlag = False
            for as_latencyTime in as_latency:
                as_latencyTotal += int(as_latencyTime)
                if int(as_latencyTime) >= self.GPIO5THRESHOLD:
                    badFlag = True
                if int(as_latencyTime) > maximum:
                    maximum = int(as_latencyTime)
                if int(as_latencyTime) < minimum:
                    minimum = int(as_latencyTime)

            #splitString = self.packets.split(',')

            if num_zeros > 0 or badFlag == True or num_twos > 1:
                result = "failed"
            else:
                result = "passed"
            message =  "GPIO sub-test {} air speed response {} (1/2). ".format(subtest_number, result)
            #
            message += "\n>>     non-responses = {}.".format(num_zeros)
            message += "\n>>     responses = {}.".format(num_ones)
            message += "\n>>     double and triple responses = {}.".format(num_twos)
            as_latencyTotal = 0
            for as_latencyTime in as_latency:
                as_latencyTotal += int(as_latencyTime)

            message += "\n>>     average airspeed latency = {} us".format(as_latencyTotal/len(as_latency))
            message += "\n>>     maximum airspeed latency = {} us".format(maximum)
            message += "\n>>     minimum airspeed latency = {} us".format(minimum)

            #self.packet1 = ""
            #self.packet2 = ""
            #self.packet3 = ""
            #self.packet4 = ""
            #self.i = 0

            return message# + "{}".format(self.packetCount)
        elif self.i == 2:
            message = self.packet3
            self.packet3 = data
            self.i += 1;

        elif self.i == 3:
        #if self.packetCount == "1":
            self.packet4 = data
            self.i += 1;

            data1 = self.packet3.split(',')
            data2 = self.packet4.split(',')
            #self.packets = ""
            

            as_response_flags = data1 #splitString[:len(splitString)/2 + 1]
            as_latency = data2 #splitString[len(splitString)/2:]

            num_zeros = as_response_flags.count("0") 
            num_ones = as_response_flags.count("1") 
            num_twos = as_response_flags.count("2") + as_response_flags.count("3")
            # iterate through, find mean, max, threshold
            as_latencyTotal = 0
            maximum = 0
            minimum = 1000
            badFlag = False
            for as_latencyTime in as_latency:
                as_latencyTotal += int(as_latencyTime)
                if int(as_latencyTime) >= self.GPIO5THRESHOLD:
                    badFlag = True
                if int(as_latencyTime) > maximum:
                    maximum = int(as_latencyTime)
                if int(as_latencyTime) < minimum:
                    minimum = int(as_latencyTime)

            #splitString = self.packets.split(',')

            if num_zeros > 0 or badFlag == True or num_twos > 1:
                result = "failed"
            else:
                result = "passed"
            message =  "GPIO sub-test {} tranponder response {} (2/2). ".format(subtest_number, result)
            #
            message += "\n>>     non-responses = {}.".format(num_zeros)
            message += "\n>>     responses = {}.".format(num_ones)
            message += "\n>>     double and triple responses = {}.".format(num_twos)
            as_latencyTotal = 0
            for as_latencyTime in as_latency:
                as_latencyTotal += int(as_latencyTime)

            message += "\n>>     average tranponder latency = {} us".format(as_latencyTotal/len(as_latency))
            message += "\n>>     maximum tranponder latency = {} us".format(maximum)
            message += "\n>>     minimum tranponder latency = {} us".format(minimum)

            self.packet1 = ""
            self.packet2 = ""
            self.packet3 = ""
            self.packet4 = ""
            self.i = 0

            return "\n>>" + message# + "{}".format(self.packetCount)

        return ''

    def testGPIO8(self, data):
        """Determines if the result is a pass or fail for each subtest of.
        Reference is to section 2.2.2 Tests."""
        subtest_number = "8"
        message = ""
        if self.i == 0:
            message = self.packet1
            self.packet1 = data
            self.i += 1;

        elif self.i == 1:
            message = self.packet2
            self.packet2 = data
            self.i += 1;

            data1 = self.packet1.split(',')
            data2 = self.packet2.split(',')
            #self.packets = ""
            

            as_response_flags = data1 #splitString[:len(splitString)/2 + 1]
            as_latency = data2 #splitString[len(splitString)/2:]

            num_zeros = as_response_flags.count("0") 
            num_ones = as_response_flags.count("1") 
            num_twos = as_response_flags.count("2") + as_response_flags.count("3")
            # iterate through, find mean, max, threshold
            as_latencyTotal = 0
            maximum = 0
            minimum = 1000
            badFlag = False
            for as_latencyTime in as_latency:
                as_latencyTotal += int(as_latencyTime)
                if int(as_latencyTime) >= self.GPIO5THRESHOLD:
                    badFlag = True
                if int(as_latencyTime) > maximum:
                    maximum = int(as_latencyTime)
                if int(as_latencyTime) < minimum:
                    minimum = int(as_latencyTime)

            #splitString = self.packets.split(',')

            if num_zeros > 0 or badFlag == True or num_twos > 1:
                result = "failed"
            else:
                result = "passed"
            message =  "GPIO sub-test {} air speed response {} (1/2). ".format(subtest_number, result)
            #
            message += "\n>>     non-responses = {}.".format(num_zeros)
            message += "\n>>     responses = {}.".format(num_ones)
            message += "\n>>     double and triple responses = {}.".format(num_twos)
            as_latencyTotal = 0
            for as_latencyTime in as_latency:
                as_latencyTotal += int(as_latencyTime)

            message += "\n>>     average airspeed latency = {} us".format(as_latencyTotal/len(as_latency))
            message += "\n>>     maximum airspeed latency = {} us".format(maximum)
            message += "\n>>     minimum airspeed latency = {} us".format(minimum)

            #self.packet1 = ""
            #self.packet2 = ""
            #self.packet3 = ""
            #self.packet4 = ""
            #self.i = 0

            return message# + "{}".format(self.packetCount)
        elif self.i == 2:
            message = self.packet3
            self.packet3 = data
            self.i += 1;

        elif self.i == 3:
        #if self.packetCount == "1":
            self.packet4 = data
            self.i += 1;

            data1 = self.packet3.split(',')
            data2 = self.packet4.split(',')
            #self.packets = ""
            

            as_response_flags = data1 #splitString[:len(splitString)/2 + 1]
            as_latency = data2 #splitString[len(splitString)/2:]

            num_zeros = as_response_flags.count("0") 
            num_ones = as_response_flags.count("1") 
            num_twos = as_response_flags.count("2") + as_response_flags.count("3")
            # iterate through, find mean, max, threshold
            as_latencyTotal = 0
            maximum = 0
            minimum = 1000
            badFlag = False
            for as_latencyTime in as_latency:
                as_latencyTotal += int(as_latencyTime)
                if int(as_latencyTime) >= self.GPIO5THRESHOLD:
                    badFlag = True
                if int(as_latencyTime) > maximum:
                    maximum = int(as_latencyTime)
                if int(as_latencyTime) < minimum:
                    minimum = int(as_latencyTime)

            #splitString = self.packets.split(',')

            if num_zeros > 0 or badFlag == True or num_twos > 1:
                result = "failed"
            else:
                result = "passed"
            message =  "GPIO sub-test {} tranponder response {} (2/2). ".format(subtest_number, result)
            #
            message += "\n>>     non-responses = {}.".format(num_zeros)
            message += "\n>>     responses = {}.".format(num_ones)
            message += "\n>>     double and triple responses = {}.".format(num_twos)
            as_latencyTotal = 0
            for as_latencyTime in as_latency:
                as_latencyTotal += int(as_latencyTime)

            message += "\n>>     average tranponder latency = {} us".format(as_latencyTotal/len(as_latency))
            message += "\n>>     maximum tranponder latency = {} us".format(maximum)
            message += "\n>>     minimum tranponder latency = {} us".format(minimum)

            self.packet1 = ""
            self.packet2 = ""
            self.packet3 = ""
            self.packet4 = ""
            self.i = 0

            return "\n>>" + message# + "{}".format(self.packetCount)

        return ''

    def testGPIO9(self, data):
        """Determines if the result is a pass or fail for each subtest of.
        Reference is to section 2.2.2 Tests."""
        subtest_number = "9"
        message = ""

        airspeed = data[6:12]
        alt = data[17:] #altitude
        #data = resultString.replace(" ", "")[2:-1] # remove white space
        
        if data[0:2] == "TP": # check that it is in status mesasge format
            if len(data) == 20:
                result = "passed"
            else:
                result = "failed"
            message =  "UART sub-test {} {}. Length = {}.".format(subtest_number, result, len(data))
            message += "\n>>     airspeed = {} m/s".format(airspeed)
            message += "\n>>     altitude = {} m".format(alt)
        else:
            message =  "UART sub-test {} failed. 'ST' identifier not present.".format(subtest_number)
        #message = "{}".format(infoList)

        return "\n>>" + message

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
        if subtest_number == "5":
            
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
        elif subtest_number == "6":
            message =  "GPIO sub-test {}.".format(subtest_number)
        ##############################################################
        # in case subtest number is not recognised.
        else:
            message =  "Invalid UART subtest number ({}) received.".format(subtest_number)
        return message

    def interpretTest(self, resultString):
        """interprets test results. This assumes that the start character, 
        $, has been removed from resultString string."""
        
        subtest_num = resultString[0]
        dataString = resultString[2:-1] # remove the unessacry ';' and test number.
        message = ""

        if self.currentTest == self.UART:
            message = self.interpretUARTSubTests(resultString)
        elif self.currentTest == self.GPIO:
            message = self.interpretGPIOSubTests(resultString)
        elif self.currentTest == self.BLANK:
            message = "BLANK Test."
        elif self.currentTest == self.COMBINED:
            if subtest_num == "0":
                message = self.testUART0(dataString)
            elif subtest_num == "1":
                #message = self.testUART1(dataString)
                message = "Test 1 spec not provided."
            elif subtest_num == "2":
                message = self.testUART2(dataString)
            elif subtest_num == "3":
                message = self.testUART3(dataString)
            elif subtest_num == "4":
                message = self.testUART4(dataString)
            elif subtest_num == "5":
                message = self.testGPIO5(dataString)
            elif subtest_num == "6":
                message = self.testGPIO6(dataString)
            elif subtest_num == "7":
                message = self.testGPIO7(dataString)
            elif subtest_num == "8":
                message = self.testGPIO8(dataString) #
            elif subtest_num == "9":
                message = self.testGPIO9(dataString)
            elif subtest_num == "E":
                return ""
                #message = self.testErrorCodes(dataString)
            else:
                message = "Invalid subtest number ({}) received.".format(subtest_num)
        else:
            message = "Internal Error. Could not interpret Test."
        return message #+ "{}".format(self.currentTest)

    

    #def startESTR(self) # start the ESTR if it has been stopped or paused.
    #def stopESTR(self) # stops the ESTR from running.
    #gdef getDeviceStatus(self) # return the status of the ESTR.

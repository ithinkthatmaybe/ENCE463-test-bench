# This is the main file that runs the test manager.
#
# This is code that loads binary files on to the Stellaris
# using "LM Flash Programmer". This code assumes that the
# programmer is located in "C:\LMFlash.exe". You can 
# download it from the TI website. 
#
# This code was written in python 2.7. You may need to install
# pyserial if you haven't already.
#
# Author: Martin Steinke 2015.



# for a nicer print function that doesn't make a new line
from __future__ import print_function 

# import the class that interfaces with the programmer and
# interprets the tests.
from loadTest import LoadTest 

# import my datalog file manager class
from fileManager import FileManager

from comms import Comms # import serial communications
import Tkinter as tk # import python's GUI manager
import msvcrt # import python's keyboard/text inerface
import os # user this to get current working directory
import time # python's time library
import threading # python's multithreading library
import Queue # queue library that is used in conjuntion with threading library


# create an event that signals that the user wants to quit.
quit_event = threading.Event()

def closeWindow():
    """This is called by Tkinter when that COMMS window is exited."""
    root.destroy()
    quit_event.set()
    print("Quit button pressed")

# create python Tkinter grapical user interface for COMM port viewer
root = tk.Tk()
root.protocol("WM_DELETE_WINDOW", closeWindow)
root.wm_title("COM Port Debug Monitor")
root.geometry("300x600")
COMframe = tk.Frame(root, width=200, height=100, bg = '#ffffff', borderwidth=1, relief="sunken")
scrollbar = tk.Scrollbar(COMframe)
editArea = tk.Text(COMframe, width=200, height=100, yscrollcommand=scrollbar.set, borderwidth=0, highlightthickness=0)
scrollbar.config(command=editArea.yview)
scrollbar.pack(side="right", fill="y")
editArea.pack(side="left", fill="both", expand=True)
COMframe.pack()
#editArea.insert(tk.END, "Hello World!\n")

def guiAddText(string):
    """A function that makes it convientant to insert text
    to the GUI."""
    #editArea.config(state=tk.NORMAL)
    editArea.insert(tk.END, string)
    #editArea.config(state=tk.DISABLED)
    #editArea.see(tk.END)

def printMenu():
    """display the help menu in the text userinterface"""
	# Display some instructions
	print("\n")
	print("-------------------------------------------------------------\n")
	print("Welcome to Group 06's basic UUT test manger!\n")
        print("press: p to program ESTR.")
	print("       h for this menu.")
	print("       r to reset device.")
	print("       s[test_num] to order test.")
	print("       q to quit.\n")
	#print("-------------------------------------------------------------")
        print("List of available tests:")
        print("       0 mirror TX")
        print("       1 mirror RX (ommitted)")
        print("       2 status message")
        print("       3 emergency test")
        print("       4 proccessor clock variation test")
        print("       5 typical airspeed operation")
        print("       6 stressing airspeed operation")
        print("       7 synchronous airspead transponder interference")
        print("       8 asychronous airspeed transponder interference")
        print("       9 transponder response verification\n")
        print("-------------------------------------------------------------\n")

def interpretCommand(command_chr):
    """decide which action to make given the user input. Actions include
    reseting the device, loading different programs, displaying the help menu
    and exiting the program."""
    # Check if input is valid.
    if command_chr not in ["p", "h", "r", "q", "s", "\n"]:
        print("Invalid Entry!")
    # Load the test onto Stellaris if input is valid.
    elif command_chr == "a":
        _path = os.getcwd() + "/testBins/uart.bin"
        ESTR.currentTest = ESTR.UART
        ESTR.loadTest(_path)
        
    elif command_chr == "b":
        _path = os.getcwd() + "/testBins/gpio.bin"
        ESTR.currentTest = ESTR.GPIO
        ESTR.loadTest(_path)
        
    elif command_chr == "3":
        _path = os.getcwd() + "/testBins/test_blank_1.bin"
        ESTR.currentTest = ESTR.BLANK
        ESTR.loadTest(_path)
        
    elif command_chr == "4":
        _path = os.getcwd() + "/testBins/test_blank_2.bin"
        ESTR.currentTest = ESTR.BLANK
        ESTR.loadTest(_path)
    elif command_chr == "p":
        _path = os.getcwd() + "/testBins/combined.bin"
        ESTR.currentTest = ESTR.COMBINED
        ESTR.loadTest(_path)
        
    elif command_chr == "r":
        ESTR.resetESTR()
    elif command_chr in ["m", "h"]:
    	printMenu()
    elif command_chr =="q":
        quit_event.set()
    else:
        pass
    

# Read COM port and add input to GUI. Producer thread
def readCOM():
    """read bytes from the COM port and add it to a buffer/queue.
    This is implemented as a 'producer' thread"""
    while True:
        if COM.inWaiting():
            s = COM.readChar()
            guiAddText(s)
            #dataLog.write(s)
            COMQueue.put(s)

def COMWorker():
    """This works on the data from the COM port via the queue. Work includes
    recording to a log file, displaying to the screen and interpreting 
    information packets from the ESTR. This is implemented as a 'worker' thread"""
    while True:
        item = COMQueue.get() # pop data from COM port
        dataLog.write(item) # record data to file

        # -check if start character, $, is received.
        # -get next character which is test number.
        # -pass on to test specific function.
        resultString = ""
        if item == ESTR.STARTCHAR:
            #subtest_number = COMQueue.get()
            semicolon_count = 0
            resultString = ""
            # loop until a full data packet has been recieved
            while semicolon_count < 2:
                newChar = COMQueue.get()

                #remove those pesky null bytes
                if newChar != "\0":
                    resultString += newChar

                # count two semicolons as packet terminators
                if newChar == ";":
                    semicolon_count += 1

            # process the test
            message = ESTR.interpretTest(resultString)
            print(message, end='') # print result to TUI
            dataLog.write(resultString) #record to rawlog
            resultLog.write(message) # record to seperate results log
        COMQueue.task_done() # done working.


# read user input
def readTUI():
    """Extract user commands from the command line interface and
    act on them. This is implemented as thread."""
    while True:
        if msvcrt.kbhit(): # wait for a char press
            user_char = msvcrt.getch() # pop the char that was pressed
            # Check input to see if user wants to quit.
            if user_char == "q": 
                quit_event.set()

            # Check if input is valid and act on it.
            interpretCommand(user_char)
            if user_char == "s":
                print("\nEnter test number: ".format(user_char), end='')
                item = msvcrt.getch()
                print("{}".format(item))
                # reset the ESTR and wait for it to reboot.
                # we had issues with this mainly for test 9.
                ESTR.resetESTR() 
                time.sleep(0.5)
                # send the desired test number to the ESTR
                COM.sendStr(item)

################################ MAIN LOOP ##########################################
printMenu()

# Make an object to represent the Stellaris and reset device.
ESTR = LoadTest()
ESTR.resetESTR()

# Create two log files. One for raw COM port data, the other
# for interpreted test results.
dataLog = FileManager("logs/datalog{}.txt")
resultLog = FileManager("logs/resultlog{}.txt")

#try to open the COM port to connect to ESTR
try:
    COM = Comms("COM39", 115200*2) #57600 #203400
except:
    quit_event.set()
    print("COM port could not be opened! Exiting...")

# Create a queue to communicate between COM producer thread
# and COM worker thread.
COMQueue = Queue.Queue()

# Create three daemom threads, one to read the COM port, one to 
# work on the COM data and the last to read the command line interface
readCOMThread = threading.Thread(target=readCOM, name="readCOM")
readCOMThread.daemon = True
readCOMThread.start()
COMWorkerThread = threading.Thread(target=COMWorker, name="COMWorker")
COMWorkerThread.daemon = True
COMWorkerThread.start()
readTUIThread = threading.Thread(target=readTUI, name="readTUI")
readTUIThread.daemon = True
readTUIThread.start()


# Loop around and update the tkinter graphical user interface
while not quit_event.isSet():
    root.update()

# print exit message when program ended
print("That's all folks!")


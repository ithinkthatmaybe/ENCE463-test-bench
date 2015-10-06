# Python 2.7 code to load binary files on to the Stellaris
# using "LM Flash Programmer". This code assumes that the
# programmer is located in "C:\LMFlash.exe". You can 
# download it from the TI website.
#
# Author: Martin Steinke 2015.



# for a nicer print function that doesn't make a new line
from __future__ import print_function 

# import the class that interfaces with the programmer.
from loadTest import LoadTest 
from fileManager import FileManager
from comms import Comms
import Tkinter as tk
import msvcrt
import os
import time
import threading
import Queue


quit_event = threading.Event()
enterCommand_event = threading.Event()
enterCommand_event.clear()
taskCompleted_event = threading.Event()
doAllTests_event= threading.Event() 

def closeWindow():
    root.destroy()
    quit_event.set()
    print("Quit button pressed")

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
    #editArea.config(state=tk.NORMAL)
    editArea.insert(tk.END, string)
    #editArea.config(state=tk.DISABLED)
    #editArea.see(tk.END)
    pass

def printMenu():
	# Display some instructions
	print("\n")
	print("-------------------------------------------------------------\n")
	print("Welcome to Group 06's basic UUT test manger!\n")
	#print("press: 1 for UART Mirror test. ")
	#print("       2 for GPIO test.")
	#print("       3 for Blank 1 test.")
	#print("       4 for Blank 2 test.")
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
    #enterCommand_event.set()
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
    #elif command_chr =="f":
    #    doAllTests_event.set()
    else:
        pass
    #enterCommand_event.set()
    

# Read COM port and add input to GUI. Producer thread
def readCOM():
    while True:
        if COM.inWaiting():
            s = COM.readChar()
            guiAddText(s)
            #dataLog.write(s)
            COMQueue.put(s)

def COMWorker():
    while True:
        item = COMQueue.get()
        dataLog.write(item)
        # -check if start character, $, is received.
        # -get next character which is test number.
        # -pass on to test specific function.
        # -end
        resultString = ""
        if item == ESTR.STARTCHAR:
            #subtest_number = COMQueue.get()
            semicolon_count = 0
            resultString = ""
            while semicolon_count < 2:
                newChar = COMQueue.get()
                #remove those pesky null bytes
                if newChar != "\0":
                    #print("ARGH!")
                    resultString += newChar
                if newChar == ";":
                    semicolon_count += 1
                #COMQueue.task_done()

            
            
            message = ESTR.interpretTest(resultString)
            print(message)
            #TUIQueue.put(message )
            #dataLog.write(message)
            dataLog.write(resultString)
            resultLog.write(message)
            #TUIQueue.put(resultString)
            #enterCommand_event.clear()
            #COMQueue.task_done()
        COMQueue.task_done()


# read user input
def readTUI():
    while True:
        if msvcrt.kbhit():
            user_char = msvcrt.getch()
            #print("{}".format(user_char))
            #print(user_char.encode('hex'))
            #guiAddText("<{}>\n".format(user_char))
            # Check input to see if user wants to quit.
            if user_char == "q": 
                quit_event.set()
            # Check if input is valid.
            #else:
            enterCommand_event.set()
            interpretCommand(user_char)
            if user_char == "s":

                print("\nEnter test number: ".format(user_char), end='')
                item = msvcrt.getch()
                print("{}".format(item))
                if item == "7":
                    ESTR.resetESTR()
                    time.sleep(0.5)
                COM.sendStr(item)
            #else:
            #    enterCommand_event.clear()
            #while enterCommand_event.isSet():
            #    pass
            
            #print("\nEnter Command: ".format(user_char), end='')
            #enterCommand_event.clear()
        #user_input = raw_input("\nEnter Command: ")
        #interpretCommand(user_input)
        #if not TUIQueue.empty():
        #    print(TUIQueue.get())
        #    TUIQueue.task_done()


printMenu()
#print("\nEnter Command: ", end='')


# Make an object to represent the Stellaris.
ESTR = LoadTest()
ESTR.resetESTR()
dataLog = FileManager("logs/datalog{}.txt")
resultLog = FileManager("logs/resultlog{}.txt")
try:
    COM = Comms("COM39", 115200*2) #57600 #203400
except:
    quit_event.set()
    print("COM port could not be opened! Exiting...")
COMQueue = Queue.Queue()
TUIQueue = Queue.Queue()

readCOMThread = threading.Thread(target=readCOM, name="readCOM")
readCOMThread.daemon = True
readCOMThread.start()
COMWorkerThread = threading.Thread(target=COMWorker, name="COMWorker")
COMWorkerThread.daemon = True
COMWorkerThread.start()
readTUIThread = threading.Thread(target=readTUI, name="readTUI")
readTUIThread.daemon = True
readTUIThread.start()


# Loop around; asking for input from the user.
task_counter = 0
while not quit_event.isSet():
    root.update()
    #if doAllTests_event.isSet():
    #    if taskCompleted_event.isSet():
    #        COM.sendStr(str(task_counter))
    #        task_counter += 1
    #        time.sleep(0.5)
    #        taskCompleted_event.clear()
# Done	
print("That's all folks!")


# while char not == 0x0d:
#    get more chars
#
# quit when enter is pressed

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
editArea.insert(tk.END, "Hello World!\n")

def guiAddText(string):
    #editArea.config(state=tk.NORMAL)
    editArea.insert(tk.END, string)
    #editArea.config(state=tk.DISABLED)
    editArea.see(tk.END)

def printMenu():
	# Display some instructions
	print("\n")
	print("-------------------------------------------------------------\n")
	print("Welcome to Group 06's basic UUT test manger!\n")
	print("press: 1 for UART Mirror test. ")
	print("       2 for GPIO test.")
	print("       3 for Blank 1 test.")
	print("       4 for Blank 2 test.")
	print("       m or h for this menu.")
	print("       r to reset device.")
	print("       s to send one character to COM port")
	print("       q to quit.\n")
	print("-------------------------------------------------------------\n")

def interpretCommand(command_chr):
    # Check if input is valid.
    if command_chr not in ["1", "2", "3", "4", "m", "h", "r", "q", "s", "\n"]:
        print("Invalid Entry!")
    # Load the test onto Stellaris if input is valid.
    elif command_chr == "1":
        _path = os.getcwd() + "/testBins/uart.bin"
        ESTR.loadTest(_path)
        ESTR.currentTest = ESTR.UART1
    elif command_chr == "2":
        _path = os.getcwd() + "/testBins/gpio.bin"
        ESTR.loadTest(_path)
        ESTR.currentTest = ESTR.GPIO
    elif command_chr == "3":
        _path = os.getcwd() + "/testBins/test_blank_1.bin"
        ESTR.loadTest(_path)
        ESTR.currentTest = ESTR.BLANK
    elif command_chr == "4":
        _path = os.getcwd() + "/testBins/test_blank_2.bin"
        ESTR.loadTest(_path)
        ESTR.currentTest = ESTR.BLANK
    elif command_chr == "r":
        ESTR.resetESTR()
    elif command_chr =="m":
    	printMenu()
    elif command_chr =="h":
        printMenu()
    elif command_chr =="q":
        quit_event.set()
    else:
        pass
    

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
        if item == ESTR.STARTCHAR:
            #subtest_number = COMQueue.get()
            semicolon_count = 0
            resultString = ""
            while semicolon_count < 2:
                newChar = COMQueue.get()
                resultString += newChar
                if newChar == ";":
                    semicolon_count += 1

            COMQueue.task_done()
            dataLog.write(resultString)
            print(ESTR.interpretTest(resultString) )
        #COMQueue.task_done()


# read user input
def readTUI():
    while True:
        if msvcrt.kbhit():
            user_char = msvcrt.getch()
            print("{}".format(user_char))
            #print(user_char.encode('hex'))
            #guiAddText("<{}>\n".format(user_char))
            # Check input to see if user wants to quit.
            #if user_char == "q": 
            #    quit_event.set()
            # Check if input is valid.
            #else:
            interpretCommand(user_char)
            if user_char == "s":
                print("\nEnter Character: ".format(user_char), end='')
                item = msvcrt.getch()
                print("{}".format(item))
                COM.sendStr(item)
            print("\nEnter Command: ".format(user_char), end='')
        #user_input = raw_input("\nEnter Command: ")
        #interpretCommand(user_input)


printMenu()
print("\nEnter Command: ", end='')

# Make an object to represent the Stellaris.
ESTR = LoadTest()
dataLog = FileManager("logs/datalog{}.txt")
try:
    COM = Comms("COM41", 115200)
except:
    quit_event.set()
    print("COM port could not be opened! Exiting...")
COMQueue = Queue.Queue()

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
while not quit_event.isSet():
    root.update()
# Done	
print("That's all folks!")


# while char not == 0x0d:
#    get more chars
#
# quit when enter is pressed
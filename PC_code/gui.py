# Python 2.7 code to load binary files on to the Stellaris
# using "LM Flash Programmer". This code assumes that the
# programmer is located in "C:\LMFlash.exe". You can 
# download it from the TI website.
#
# Author: Martin Steinke 2015.



# for a nicer print function that doesn't make a new line
from __future__ import print_function 

# import the class that interfaces with the programmer.
from load_test import load_test 

from comms import comms
import Tkinter as tk
import msvcrt
import os


bool_gui_exited = False

def closeWindow():
    root.destroy()
    bool_gui_exited = True
    print("Quit button pressed")

root = tk.Tk()
root.protocol("WM_DELETE_WINDOW", closeWindow)
root.wm_title("COM Port Debug Monitor")
root.geometry("300x100")
COMframe = tk.Frame(root, width=200, height=100, bg = '#ffffff', borderwidth=1, relief="sunken")
scrollbar = tk.Scrollbar(COMframe)
editArea = tk.Text(COMframe, width=200, height=100, yscrollcommand=scrollbar.set, borderwidth=0, highlightthickness=0)
scrollbar.config(command=editArea.yview)
scrollbar.pack(side="right", fill="y")
editArea.pack(side="left", fill="both", expand=True)
COMframe.pack()
editArea.insert(tk.END, "Hello World!\n")

def guiAddText(string):
    editArea.config(state=tk.NORMAL)
    editArea.insert(tk.END, string)
    editArea.config(state=tk.DISABLED)
    editArea.see(tk.END)

# Make an object to represent the Stellaris.
ESTR = load_test()
COM = comms("COM35", 115200)

def printMenu():
	# Display some instructions
	print("\n")
	print("-------------------------------------------------------------\n")
	print("Welcome to Group 06's basic UUT test manger!\n")
	print("press: 1 for UART Mirror test. ")
	print("       2 for GPIO test.")
	print("       3 for Blank 1 test.")
	print("       4 for Blank 2 test.")
	print("       m for this menu.")
	print("       r to reset device.")
	print("       q to quit.\n")
	print("-------------------------------------------------------------\n")

def interpretCommand(command_chr):
    # Check if input is valid.
    if command_chr not in ["1", "2", "3", "4", "m", "r", "q"]:
        print("Invalid Entry!")
    # Load the test onto Stellaris if input is valid.
    elif command_chr == "1":
        _path = os.getcwd() + "/testBins/uart.bin"
        ESTR.loadTest(_path)
    elif command_chr == "2":
        _path = os.getcwd() + "/testBins/gpio.bin"
        ESTR.loadTest(_path)
    elif command_chr == "3":
        _path = os.getcwd() + "/testBins/test_blank_1.bin"
        ESTR.loadTest(_path)
    elif command_chr == "4":
        _path = os.getcwd() + "/testBins/test_blank_2.bin"
        ESTR.loadTest(_path)
    elif command_chr == "r":
        ESTR.resetESTR()
    elif command_chr =="m":
    	printMenu()





# Display some instructions
printMenu()


# Loop around; asking for input from the user.
print("\nEnter Command: ", end='')
while True:
    # Get user input.
    if msvcrt.kbhit():
        user_char = msvcrt.getch()
        print("{}".format(user_char))
        #guiAddText("<{}>\n".format(user_char))
        # Check input to see if user wants to quit.
        if user_char == "q": 
            break
        # Check if input is valid.
        else:
            interpretCommand(user_char)
        print("\nEnter Command: ".format(user_char), end='')
            
    if COM.inWaiting():
        s = "{}".format(COM.readChar())
        guiAddText(s)
    root.update()
# Done	
print("That's all folks!")



# @created 25-8-2015
# @author MCS
# @description comms.py module for high level communications between ESTR and PC

import serial # for UART hardware abstraction.


class Comms(object):
	def __init__(self, COM_port, baudrate): # initialise the COMs port on the PC.
		"""initialise the COMs port on the PC, and opens it."""
		self.ser = serial.Serial()
		self.ser.port = COM_port
		self.ser.baudrate = baudrate
		self.ser.timeout = 0
		self.ser.parity = serial.PARITY_EVEN
		self.ser.open()

	def sendStr(self, str):
		"""Write a string to the COM port."""
		self.ser.write(str)

	def readChar(self):
		"""Read a character from the COM port."""
		return self.ser.read()

	def openCOMPort(self):
		"""Open the COM port."""
		self.ser.open()

	def closeCOMPort(self):
		"""Close the COM port."""
		self.ser.close()
		
	def inWaiting(self):
		"""return the number of bytes waiting in the COM port."""
		return self.ser.inWaiting()
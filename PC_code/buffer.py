# @created 30-9-2015
# @author MCS
# @description buffer.py module is a simple FIFO buffer object.


class Buffer(object):
	def __init__(self, buffer_size): 
		"""initialise the buffer."""
		#self.buffer = [buffer_size*[None]]
		self.buffer = []

	def put(self, item):
		"""Write a string to the COM port."""
		self.buffer.append(item)

	def pop(self):
		"""Return a single item from the buffer."""
		if length(self.buffer) > 0: 
			return_item = self.buffer.pop()
		else:
			return_item = None
		return return_item

	def read(self):
		"""return the entire contents of the buffer as an array"""
		return_items = []
		while length(self.buffer) > 0:
			return_items.append(self.buffer.pop())
		return return_items

	def readStr(self):
		"""return the entire contents of the buffer as a string"""
		return_items = ""
		while length(self.buffer) > 0:
			return_items + self.buffer.pop()
		return return_items


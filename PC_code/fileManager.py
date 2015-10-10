# @created 3-10-2015
# @author MCS
# @description fileManager.py module for making datalogs

import os # for accessing file directories.
import datetime # to add time stamp to log files

class FileManager(object):
    def __init__(self, path): 
        """initialise the datalog file."""
        log_num = 0
        self.path = path.format(log_num)
        # loop until a new numbered file is availiable
        while os.path.exists(self.path):
            log_num += 1
            self.path = path.format(log_num)
            
        # write a time stamp so that use knows when the file was written.
        now = str(datetime.datetime.now())
        with open(self.path, 'w') as dataFile:
            dataFile.write("This file was created on: " + now + ".\n")

    def write(self, s):
        """Append a string to the file."""
        with open(self.path, 'a') as dataFile:
            dataFile.write(s)

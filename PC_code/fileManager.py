# @created 3-10-2015
# @author MCS
# @description fileManager.py module for making a datalog
import os # for accessing file directories.


class FileManager(object):
    def __init__(self, path): 
        """initialise the file."""
        log_num = 0
        #self.path = "logs/datalog{}.txt".format(log_num)
        self.path = path.format(log_num)
        while os.path.exists(self.path):
            log_num += 1
            #self.path = "logs/datalog{}.txt".format(log_num)
            self.path = path.format(log_num)

    def write(self, s):
        """Append a string to the file."""
        with open(self.path, 'a') as dataFile:
            dataFile.write(s)

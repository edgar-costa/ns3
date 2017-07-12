import csv

file_format = {"id":0, "size": 1, "fct": 2, "startTime":3, "endTime":4}

class Fct(object):

    def __init__(self,file_name):
        self._file = file_name
        #load data
        self.raw = self.load_fct(self._file)

    def load_fct(self,file_name):
        file = open(file_name, 'rb')
        data = csv.reader(file, delimiter=" ")
        return [x for x in data]


    def create_id_to_data(self):
        pass


    def get_attribute(self,name):

        index = file_format[name]

        return [x[index] for x in self.raw]

        
    

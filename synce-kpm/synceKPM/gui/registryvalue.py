class RegistryValue:
    def __init__(self,  value_name, value_type, value_data):
        self.value_name = value_name
        self.value_type = value_type
        self.value_data = value_data

    def myformat(self, _number):
        result = hex(_number)[2:]
        if len(result) == 1:
            result = "0%s"%result
        return result ; 

    

    def value_data_str(self):
        if self.value_type == 0:
            return "" 
        if self.value_type == 1:
            return self.value_data
        if self.value_type == 2:
            return self.value_data
        if self.value_type == 3:
            return ' '.join(self.myformat(_num) for _num in self.value_data)
        if self.value_type == 4:
            return str( self.value_data )
        if self.value_type == 5:
            return str( self.value_data )
        if self.value_type == 6:
            return self.value_data
        if self.value_type == 7:
            return "Not supported - multiple strings"

    

    

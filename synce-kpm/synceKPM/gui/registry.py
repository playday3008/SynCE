from synceKPM.gui.registrykey import RegistryKey
from synceKPM.gui.registryvalue import RegistryValue

from PyQt4.QtCore import QObject

class Registry(QObject):
    def __init__(self, main_window): 
        QObject.__init__(self)

        #Set the main_window reference, for later communication
        #TODO: modify this towards signal / slots
        self.main_window = main_window
        
       
        #Declare the two models for the keys and the values
        self.registry_key_model = None
        self.registry_value_model = None
        
        self.number_of_keys_being_fetched = 0 


        self.number_of_waiting_key_requests = 0 

        self.id_to_key_map = {}

        self.currently_selected_key_id = -1 
        self.currently_selected_key = None

        self.reg_type_desc = { 0:"NONE" , 1:"SZ", 2:"EXPAND_SZ", 3:"BINARY", 4:"DWORD", 5:"DWORD (Big Endian)", 6:"LINK", 7:"MULTI_SZ"}


        self.values_in_selected_key = []

         #Create the root key, containing the header info
        self.root_item = RegistryKey( self, "", None, False)

        self.device_root_item = RegistryKey(self, "DEVICE_NAME", self.root_item,False)
        self.clear_registry_model() 



    def clear_registry_model(self):
        self.device_root_item.clear_keys() 
        self.HKCR_root_item = RegistryKey(self, "HKEY_CLASSES_ROOT",  self.device_root_item)
        self.HKCU_root_item = RegistryKey(self, "HKEY_CURRENT_USER",  self.device_root_item)
        self.HKLM_root_item = RegistryKey(self, "HKEY_LOCAL_MACHINE", self.device_root_item)

        self.values_in_selected_key = [] 
        if not self.registry_value_model is None:
            self.registry_value_model.reset() 


   
    def set_current_selected_key(self, key):
        self.currently_selected_key_id = id( key )
        self.currently_selected_key = key
        
        #Put one value there, namely: fetching...
        #and fire a reset for the views

        self.request_retrieve_values( key ) 
        
        pass


    def get_key_from_id( self, key_id ):
        return self.id_to_key_map[ key_id ]
        
    
    def get_key_from_index( self, key_index ):
        return key_index.internalPointer()



    def fetching_key(self):
        self.number_of_keys_being_fetched += 1 
        self.main_window.set_cursor( True )

    #SLOTS, to be called when the user expands / clicks on a key in the QTreeView
    def request_retrieve_keys( self, key):
        if key ==  self.HKCR_root_item:
            self.main_window.issue_warning_slow_HKCR()

        self.main_window.guiDbus.query_registry_keys( id(key), key.full_path)
        pass


    def request_retrieve_values( self, key):
        self.main_window.guiDbus.query_registry_values( id(key), key.full_path)
        pass




    #SLOTS, to be called when the guiDbus sends the signal it is finished with fetching information
    #This means that the information for that particular key can be reset now
    def retrieve_keys_finished(self, key_id):
        key = self.get_key_from_id( key_id )
        key.remove_fetching_key() ; 
        self.number_of_keys_being_fetched -= 1
        if self.number_of_keys_being_fetched == 0:
            self.main_window.set_cursor(False) 
        pass

    def retrieve_values_finished(self, key_id):
        pass

    

    #SLOTS, to be called when the guiDbus sends the signal containing all the key / value information
    def update_keys(self, key_id, subkeys_and_child_count ):
        key = self.get_key_from_id( key_id )

        if len( subkeys_and_child_count) > 0:
            self.begin_insert_keys(key, len(key.keys), len(key.keys) + len(subkeys_and_child_count) ) 

            for (sub_key,child_count) in subkeys_and_child_count:
                #In case child_count == -1 and >= 1, we must add fetching node, otherwise, not needed
                must_add_fetch_child = child_count != 0
                foo = RegistryKey( self, sub_key, key, must_add_fetch_child)

            
            key.needs_fetching = False

            self.end_insert_keys()

        else:
            key.needs_fetching = False
        
        
    def update_values(self, key_id, value_tuples, list_numerical_data, list_string_data, list_binary_data, list_binary_data_from, list_binary_data_to):
        self.values_in_selected_key = [] 
        for (value_name, value_type, value_data_type, value_data_index) in value_tuples:
            if value_data_type == 0:
                self.values_in_selected_key.append( RegistryValue(value_name, value_type , list_string_data[value_data_index])) 
            if value_data_type == 1:
                self.values_in_selected_key.append( RegistryValue(value_name, value_type , list_numerical_data[value_data_index] )) 
            if value_data_type == 2:
                binary_data = list_binary_data[ list_binary_data_from[value_data_index] : list_binary_data_to[value_data_index] ]
                self.values_in_selected_key.append( RegistryValue(value_name, value_type , binary_data ))
            
        self.registry_value_model.reset()
        self.main_window.registryValueView.resizeColumnToContents(2) 
        pass





    def begin_insert_keys(self, key, start_index, end_index):
        if self.registry_key_model:
            self.registry_key_model.beginInsertRows( self.registry_key_model.createIndex( key.row(), 0, key ), start_index, end_index)

    def end_insert_keys(self):
        if self.registry_key_model:
            self.registry_key_model.endInsertRows()


    def begin_remove_keys(self, key, start_index, end_index):
        if self.registry_key_model:
            self.registry_key_model.beginRemoveRows( self.registry_key_model.createIndex( key.row(), 0, key ), start_index, end_index)

    def end_remove_keys(self):
        if self.registry_key_model:
            self.registry_key_model.endRemoveRows()


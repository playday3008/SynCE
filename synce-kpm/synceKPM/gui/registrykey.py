class RegistryKey:
    def __init__(self, registry,  key_name, parent_key, needs_fetching=True):
        
        #Initially, set the keys and values to empty sets
        self.keys = []

        self.key_name = key_name
        self.parent = parent_key


        self.registry = registry
         
        self.registry.id_to_key_map[ id(self) ] = self 

        self.needs_fetching = needs_fetching 
       

        #Are we dealing with one of the Root keys HKCR, HKCU, HKLM?
        #This is the case when great-grandparent is None
        is_registry_root_key = False
        if not self.parent is None:
            if not self.parent.parent is None:
                if self.parent.parent.parent is None:
                    is_registry_root_key = True


        
        #In case we are the root node, just set the default path
        if parent_key is None:
            self.full_path = ""
        else:
            #Ohterwise, add ourselves to the parent_key and 
            parent_key.keys.append(self)

            #If we are HKCR,HKCU, HKLM then set the initial path now
            #Otherwise, construct the path based on the parent information
            if is_registry_root_key:
                self.full_path = "/" + self.key_name
            else:
                self.full_path = parent_key.full_path + "/" + self.key_name
             
        

        #If this key needs fetching, then add a special fetching sub_key
        if needs_fetching:
             self.add_fetching_key()

    def add_fetching_key(self):
         self.fetching_sub_key = RegistryKey( self.registry, "Fetching...", self, False)


    def append_key(self, key):
        self.registry.begin_insert_keys( self, len(self.keys), len(self.keys) )
        self.keys.append(key)
        self.registry.end_insert_keys()

    
    def row(self):
        """Return the row number of this key in the list of its parent"""
        if self.parent:
            return self.parent.keys.index(self)

        return 0



    def number_keys(self):
        return len(self.keys)


    def key(self, row):
        return self.keys[row]

    def remove_fetching_key(self):
        self.registry.begin_remove_keys( self, 0, 0 )
        self.keys = self.keys[1:] 
        self.registry.end_remove_keys() 

    def clear_keys(self, needs_fetching=False):
        self.registry.begin_remove_keys( self, 0, len(self.keys) - 1)
        self.keys = []
        self.needs_fetching = needs_fetching
        self.registry.end_remove_keys()




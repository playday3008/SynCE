from PyQt4 import QtCore

from synceKPM.gui.registrykey import RegistryKey
from synceKPM.gui.registry import Registry


class RegistryKeyModel(QtCore.QAbstractItemModel):
    def __init__(self, registry , parent=None):
        QtCore.QAbstractItemModel.__init__(self, parent)

        self.registry = registry 
        self.registry.registry_key_model = self



    def columnCount(self, parent):
        return 1


    def get_key_from_index(self, index):
        return self.registry.get_key_from_index( index )


    def data(self, index, role):
        if not index.isValid():
            return QtCore.QVariant()

        if role != QtCore.Qt.DisplayRole:
            return QtCore.QVariant()
        

        if index.column() > 0:
            return QtCore.QVariant() 

        key = self.get_key_from_index(index) 
        
        return QtCore.QVariant( "" + key.key_name )

    def flags(self, index):
        if not index.isValid():
            return QtCore.Qt.ItemIsEnabled

        return QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable


    def headerData(self, section, orientation, role):
        if orientation == QtCore.Qt.Horizontal and role == QtCore.Qt.DisplayRole:
            #return self.rootItem.data(section)
            return QtCore.QVariant( self.registry.root_item.key_name )

        return QtCore.QVariant()

    def get_key_index(self, key):
        parent_key = key.parent

        my_row = parent_key.keys.index( key )

        return self.createIndex(my_row, 0, key)


    def index(self, row, column, parent):
        myKey = self.get_key_from_index( parent )
        myPath = ""
        if not myKey is None:
            myPath = myKey.full_path 

        if row < 0 or column < 0 or row >= self.rowCount(parent) or column >= self.columnCount(parent):
            return QtCore.QModelIndex()

        if not parent.isValid():
            parent_item = self.registry.root_item
        else:
            parent_item = self.get_key_from_index(parent)

        child_item = parent_item.key(row)


        if child_item:
            index = self.createIndex(row, column, child_item)
            return index
        else:
            return QtCore.QModelIndex()



    def parent(self, index):
        if not index.isValid():
            return QtCore.QModelIndex()

        try:
            child_item = self.get_key_from_index(index) 

            parent_item = child_item.parent

            if parent_item == self.registry.root_item:
                return QtCore.QModelIndex()

            return self.createIndex(parent_item.row(), 0, parent_item)
        except KeyError:
            return QtCore.QModelIndex()

    
    def hasChildren(self, parent):
        try:
            if not parent.isValid():
                parent_key = self.registry.root_item
            else:
                parent_key = self.get_key_from_index(parent)

            return parent_key.number_keys() > 0
        except:
            return False

    def rowCount(self, parent):
        if parent.column() > 0:
            return 0


        try:
            if not parent.isValid():
                parent_key = self.registry.root_item
            else:
                parent_key = self.get_key_from_index(parent)

            return parent_key.number_keys()
        except:
            return 0


    def fetch_key_subkeys(self, key):
        if key.needs_fetching:
            key.needs_fetching = False
            self.registry.fetching_key() #This is for the busy mouse pointer
            self.registry.request_retrieve_keys( key )



    def key_expanded(self, index):
        key = self.get_key_from_index( index )

        self.fetch_key_subkeys(key)


    def key_clicked(self, index):
        key = self.get_key_from_index( index )
        if key.key_name != "Fetching..." and key != self.registry.device_root_item:
            self.registry.set_current_selected_key( key )


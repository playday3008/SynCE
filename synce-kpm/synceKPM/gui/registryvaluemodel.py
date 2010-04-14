
from PyQt4 import QtCore

from synceKPM.gui.registrykey import RegistryKey
from synceKPM.gui.registry import Registry


class RegistryValueModel(QtCore.QAbstractTableModel):
    def __init__(self, registry , parent=None):
        QtCore.QAbstractTableModel.__init__(self, parent)

        self.registry = registry 
        self.registry.registry_value_model = self


    def selected_key_changed(self):
        self.reset() 

    def columnCount(self, parent):
        return 3



    def data(self, index, role):
        if not index.isValid():
            return QtCore.QVariant()

        if role != QtCore.Qt.DisplayRole:
            return QtCore.QVariant()

        if index.column() > 3:
            return QtCore.QVariant() 

        if index.column() == 0:
            return QtCore.QVariant( self.registry.values_in_selected_key[ index.row() ].value_name    ) 

        if index.column() == 1:
            return QtCore.QVariant( self.registry.reg_type_desc[ self.registry.values_in_selected_key[ index.row() ].value_type ]   ) 
        
        if index.column() == 2:
            return QtCore.QVariant( self.registry.values_in_selected_key[ index.row() ].value_data_str()   ) 

    def headerData(self, section, orientation, role):
        if orientation == QtCore.Qt.Horizontal and role == QtCore.Qt.DisplayRole:
            #return self.rootItem.data(section)
            if section == 0:
                return QtCore.QVariant( "Name" )
            if section == 1:
                return QtCore.QVariant( "Type" )
            if section == 2:
                return QtCore.QVariant( "Data" )


    def rowCount(self, parent):
        return len( self.registry.values_in_selected_key )


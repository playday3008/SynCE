class Observable(object):
    def __init__(self):
        self.listenerList = []

    def sendMessage(self, event):
        for callback in self.listenerList:
            callback(event)
        return

    def addListener(self, callback):
        self.listenerList.append(callback)

    def removeListener(self, callback):
        self.listenerList.remove(callback)

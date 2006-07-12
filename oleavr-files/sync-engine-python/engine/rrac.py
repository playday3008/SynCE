from twisted.internet.protocol import Protocol
from util import hexdump

class BaseProtocol(Protocol):
    def connectionMade(self):
        print "%s.connectionMade: client connected" % \
            (self.__class__.__name__)

    def connectionLost(self, reason):
        print "%s.connectionLost: connection lost because: %s" % \
            (self.__class__.__name__, reason)

    def dataReceived(self, data):
        print "%s.dataReceived: got %d bytes of data" % \
            (self.__class__.__name__, len(data))
        print hexdump(data)
        print

class RRAC(BaseProtocol):
    pass

class Status(BaseProtocol):
    pass

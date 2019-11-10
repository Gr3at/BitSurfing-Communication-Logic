import socket
from collections import deque
import random

def OpenAndStoreCodebookToList(filename):
    validMsgsVector =[]
    with open(filename) as f:
        [validMsgsVector.append(line.strip('\n')) for line in f]
    return validMsgsVector

class NanoNode(object):
    def __init__(self, nodeid, buffsize=30, udpPort=37020):
        self.nodeid = nodeid
        self.x = random.randint(0,100)
        self.y = random.randint(0,100)
        self.buffsize = buffsize
        self.buffer = ""
        self.MsgFromOtherNode = False
        self.dq = deque(maxlen=buffsize)
        self.c = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)  # for UDP Socket
        self.c.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        self.c.bind(("", udpPort))
    
    def listenBitStreamStoreAndCheckBuffer(self, bytesToListen = 2):
        msg = []
        data, _ = self.c.recvfrom(bytesToListen)
        [msg.append("{0:08b}".format(byte)) for byte in data]
        msg = ''.join(msg)
        for bit in msg:
            self.dq.appendleft(bit)
            self.buffer = ''.join(self.dq)
            if self.MsgFromOtherNode == True:
                print("True!")
            
            if self.SellectRandomlyMessageOfInterest in self.buffer[:ValidMsgLen]:
                print("success! MSG %s and buffer %s" %(self.SellectRandomlyMessageOfInterest,self.buffer))
                self.raiseFlag()
        
    def listenBitStreamAndStoreToBuffer(self, bytesToListen = 2):
        msg = []
        data, _ = self.c.recvfrom(bytesToListen)
        [msg.append("{0:08b}".format(byte)) for byte in data]
        msg = ''.join(msg)
        [self.dq.appendleft(x) for x in msg]
        self.buffer = ''.join(self.dq)
        
    def validMsgInBuffer(self):
        pass
    
    def logMsgToFile(self, offset, nodeType):
        try:
            f=open("log"+str(self.nodeid)+".txt", "a+", encoding="utf-8")
            if (nodeType=="Dest"):
                f.write("NodeID : "+str(self.nodeid)+" | Message Received : " + str(self.buffer[offset:offset+ValidMsgLen])+"\n")
                print("Successfully logged data")
            elif (nodeType=="Source"):
                f.write("NodeID : "+str(self.nodeid)+" | Message Sent : " + str(self.buffer[offset:offset+ValidMsgLen])+"\n")
                print("Successfully logged data")
            else:
                f.write("NodeType not specified to 'Source' or 'Dest'.\n")            
            f.close()
        except:
            print("Failed to write log file. Check if the file is already in use")

    def searchMsgInBufferAndLogMsg(self, nodeType):
        try:
            subStrIdx = self.buffer.index(Prefix)
            if (len(self.buffer) - subStrIdx) - (len(Prefix)+2*len(self.nodeid)+1) >= ValidMsgLen:
                if (nodeType=="Dest") and (self.buffer[subStrIdx+len(Prefix)+len(self.nodeid):subStrIdx+len(Prefix)+2*len(self.nodeid)]==self.nodeid):
                    self.logMsgToFile(subStrIdx, "Dest")
                elif (nodeType=="Source") and (self.buffer[subStrIdx+len(Prefix):subStrIdx+len(Prefix)+len(self.nodeid)]==self.nodeid):
                    self.raiseFlag()
                    self.logMsgToFile(subStrIdx, "Source")
        except ValueError:
            print("no valid sequence found")
    
    def raiseFlag(self):  #TODO
        ''' write apropriate code to signal other nanoDevices for incoming message '''
        #GPIO.OUT = HIGH
        pass
    
    def checkFlag(self):  #TODO
        ''' use this method to check whether the flag was raised because of incoming message '''
        #if inputGPIO == HIGH:
        #    self.MsgFromOtherNode==True
        pass
    
    def __del__(self):
        self.c.close()

''' define the buffer size of the each nanonode in BITS (its actually not bits tpyed)!!! '''
Prefix = "1000"
bytesToListen = 2
naNode = NanoNode("10111")
validMsgsVector = OpenAndStoreCodebookToList("codebook1.txt")
naNode.SellectRandomlyMessageOfInterest = validMsgsVector[random.randint(0,2031)]
ValidMsgLen = len(naNode.SellectRandomlyMessageOfInterest)
#%%
while True:
    msg = []
    data, _ = naNode.c.recvfrom(bytesToListen)
    [msg.append("{0:08b}".format(byte)) for byte in data]
    msg = ''.join(msg)
    for bit in msg:
        naNode.dq.appendleft(bit)
        naNode.buffer = ''.join(naNode.dq)
        naNode.checkFlag()
                
        if naNode.MsgFromOtherNode==False:
            if naNode.SellectRandomlyMessageOfInterest in naNode.buffer[:ValidMsgLen]:
                print("success! MSG %s and buffer %s" %(naNode.SellectRandomlyMessageOfInterest,naNode.buffer))
                naNode.raiseFlag()
                #naNode.listenBitStreamStoreAndCheckBuffer()
                #naNode.searchMsgInBufferAndLogMsg("Source")
        else:
            #naNode.searchMsgInBufferAndLogMsg("Dest")
            naNode.MsgFromOtherNode=False

del naNode

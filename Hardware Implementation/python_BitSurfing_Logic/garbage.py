#%% there are not 32 node ID available since the prefix must not be repeated in a valid sequence

count = 0
for i in range(32):
    msg = "{0:05b}".format(i)
    print(str(i+1) + " | " + msg + " | " + str(len(msg)))
    if "1000" in msg:
        count+=1
        print("Found")

print(count)


#%% UART Communication

'''Example below may not be fully functional'''
import serial
serialData = serial.Serial('/dev/ttyACM0',115200) 
...
try:    
    # Body of data acquisition code
    while True:
        line = serialData.read(325); # stream is 325 bytes long ,  last byte is a 0x011
        # first 4 bytes are time in msec
        Ttemp = array.array("I",line[0:4]);  # extracts time the I, uint32_t millis
        currT = Ttemp[0]/1000.0; # writes single element array to variable
        #print "Time = ",currT
        # extracts the array of h, int16_t variable
        tempArray = array.array("h",line[4:324]); s
        #print "tempArray ="
        #print tempArray
except:
        print("Error")    
        
#%% threaded implementation 1

''' Draft code below demonstrates RasPi Threaded Implementation '''
''' #Threads1 : open 2+1(parent) threads to do the job '''

import socket
from collections import deque
import random
import time
import threading, queue

try:
    import Adafruit_BBIO.GPIO as GPIO
    inputChannelPin="P8_9"
    outputChannelPin="P8_7"
    GPIO.setup(inputChannelPin, GPIO.IN)
    GPIO.setup(outputChannelPin, GPIO.OUT, initial=GPIO.LOW)
    GPIO.add_event_detect(inputChannelPin, GPIO.RISING)
except:
    print("Is the script really running on Rasberry?")

def OpenAndStoreCodebookToList(filename):
    validMsgsVector =[]
    with open(filename) as f:
        [validMsgsVector.append(line.strip('\n')) for line in f]
    return validMsgsVector

class NanoNodeMT(object):
    def __init__(self, nodeid, buffsize=30, udpPort=37020):
        self.nodeid = nodeid
        self.buffer = ""
        self.dq = deque(maxlen=buffsize)
        self.c = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)  # for UDP Socket
        self.c.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        self.c.bind(("", udpPort))
        self.counter=0
        try:
            self.fin = open("incomingLog"+str(self.nodeid)+".txt", "a+", encoding="utf-8")
            self.fout=open("sendingLog"+str(self.nodeid)+".txt", "a+", encoding="utf-8")
        except:
            print("Failed to open log file. Check if the file is already in use")  
            
        
    def findValidMsgInBuffer(self):
        self.ValidMsgFound= "None"
        for subbuff in (self.buffer[i:i+self.ValidMsgLen] for i in range(len(self.buffer)-self.ValidMsgLen+1)):
            if subbuff in validMsgsVector:
                self.ValidMsgFound = subbuff
                break
    
    def logMsgToFile(self, nodeType):
        try:
            if (nodeType=="Dest"):
                self.fin.write(str(self.ValidMsgFound)+" | " + str(time.time())+"\n")
            elif (nodeType=="Source"):
                self.fout.write(str(self.randomlySelectedMessageOfInterest)+" | " + str(time.time())+"\n")
        except:
            print("Failed to write log file. Check if the file is already in use")  
    
    def raiseFlag(self):
        GPIO.output(outputChannelPin, GPIO.HIGH)
        self.counter+=1
        print(self.counter)
        GPIO.output(outputChannelPin, GPIO.LOW)

    def listenBitStream(self, q):
        while self.counter<4:
            msg = []
            data, _ = self.c.recvfrom(bytesToListen)
            [msg.append("{0:08b}".format(byte)) for byte in data]
            msg = ''.join(msg)
            for bit in msg:
                self.dq.appendleft(bit)
                tempBuffer = ''.join(self.dq)
                q.put(tempBuffer)
    
    def searchBufferAndCheckFlags(self, q):
        while self.counter<4:
            self.buffer = q.get()
            
            if GPIO.event_detected(inputChannelPin):
                self.findValidMsgInBuffer()
                self.logMsgToFile("Dest")
            else:
                if self.randomlySelectedMessageOfInterest in self.buffer[:self.ValidMsgLen]:
                    self.raiseFlag()
                    self.logMsgToFile("Source")
                    '''change the sending message of current node'''
                    self.randomlySelectedMessageOfInterest = validMsgsVector[random.randint(0,len(validMsgsVector)-1)]
                    self.ValidMsgLen = len(self.randomlySelectedMessageOfInterest)
    
    def __del__(self):
        self.c.close()
        self.fin.close()
        self.fout.close()



bytesToListen = 2
naNode = NanoNodeMT("10101")
validMsgsVector = OpenAndStoreCodebookToList("codebook1.txt")
naNode.randomlySelectedMessageOfInterest = validMsgsVector[random.randint(0,len(validMsgsVector)-1)]
naNode.ValidMsgLen = len(naNode.randomlySelectedMessageOfInterest)

q = queue.Queue()
thread1 = threading.Thread(target=naNode.listenBitStream,args=(q))
thread2 = threading.Thread(target=naNode.searchBufferAndCheckFlags,args=(q))
#%%
thread1.start()
thread2.start()
print("Waiting for all threads to terminate...")
thread1.join()
thread2.join()

del naNode
GPIO.cleanup()



#%% RasPi

import socket
from collections import deque
import random
import time

try:
    import RPi.GPIO as GPIO
    GPIO.setmode(GPIO.BOARD)
    inputChannelPin=11
    outputChannelPin=13
    GPIO.setup(inputChannelPin, GPIO.IN)
    GPIO.setup(outputChannelPin, GPIO.OUT, initial=GPIO.LOW)
    GPIO.add_event_detect(inputChannelPin, GPIO.RISING)
except:
    print("Is the script really running on Rasberry?")

def OpenAndStoreCodebookToList(filename):
    validMsgsVector =[]
    with open(filename) as f:
        [validMsgsVector.append(line.strip('\n')) for line in f]
    return validMsgsVector

class NanoNodeMT(object):
    def __init__(self, nodeid, buffsize=30, udpPort=37020):
        self.nodeid = nodeid
        self.buffsize = buffsize
        self.buffer = ""
        self.MsgFromOtherNode = False
        self.dq = deque(maxlen=buffsize)
        self.c = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)  # for UDP Socket
        self.c.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        self.c.bind(("", udpPort))
        self.counter=0
        
    def findValidMsgInBuffer(self):
        self.ValidMsgsFound= ""
        for subbuff in (self.buffer[i:i+self.ValidMsgLen] for i in range(len(self.buffer)-self.ValidMsgLen+1)):
            if subbuff in validMsgsVector:
                self.ValidMsgsFound = subbuff
                break
        print(self.ValidMsgsFound)
    
    def logBufferToFile(self):
        try:
            fb=open("logbuffer"+".txt", "a+", encoding="utf-8")
            fb.write(str(self.buffer) + " | " + str(time.ctime())+"\n")
            fb.close()
        except:
            print("Failed to write buffer to file")
    
    def logMsgToFile(self, nodeType):
        try:
            f=open("log"+str(self.nodeid)+".txt", "a+", encoding="utf-8")
            if (nodeType=="Dest"):
                f.write(" ValidMsgsFound: "+str(self.ValidMsgsFound)+" | Buffer : " + str(self.buffer)+"\n")
                print("Successfully logged data")
            elif (nodeType=="Source"):
                f.write(" RandomMsg: "+str(self.SellectRandomlyMessageOfInterest)+" | Buffer : " + str(self.buffer)+"\n")
                print("Successfully logged data")
            else:
                f.write("NodeType not specified to 'Source' or 'Dest'.\n")            
            f.close()
        except:
            print("Failed to write log file. Check if the file is already in use")  
    
    def raiseFlag(self):
        GPIO.output(outputChannelPin, GPIO.HIGH)
        #time.sleep(0.1)
        self.counter+=1
        print(self.counter)
        GPIO.output(outputChannelPin, GPIO.LOW)

    def resetFlag(self):
        GPIO.output(outputChannelPin, GPIO.LOW)
    
    def checkFlag(self):
        if GPIO.event_detected(inputChannelPin):
            self.MsgFromOtherNode=True
            print("Flag up")
    
    def __del__(self):
        self.c.close()

''' define the buffer size of the each nanonode in BITS (its actually not bits tpyed)!!! '''
Prefix = "1000"
bytesToListen = 2
naNode = NanoNodeMT("10111")
validMsgsVector = OpenAndStoreCodebookToList("codebook1.txt")
naNode.SellectRandomlyMessageOfInterest = validMsgsVector[random.randint(0,2031)]
naNode.ValidMsgLen = len(naNode.SellectRandomlyMessageOfInterest)
#%%
while True:
    if naNode.counter>4:
        break
    msg = []
    data, _ = naNode.c.recvfrom(bytesToListen)
    [msg.append("{0:08b}".format(byte)) for byte in data]
    msg = ''.join(msg)
    for bit in msg:
        naNode.dq.appendleft(bit)
        naNode.buffer = ''.join(naNode.dq)
        #naNode.logBufferToFile()
        naNode.checkFlag()
        #naNode.resetFlag()
        
        if naNode.MsgFromOtherNode==True:
            naNode.findValidMsgInBuffer()
            naNode.logMsgToFile("Dest")
            naNode.MsgFromOtherNode=False
        else:
            if naNode.SellectRandomlyMessageOfInterest in naNode.buffer[:naNode.ValidMsgLen]:
                print("success! MSG %s and buffer %s" %(naNode.SellectRandomlyMessageOfInterest,naNode.buffer))
                naNode.raiseFlag()
                naNode.logMsgToFile("Source")
        
del naNode
GPIO.cleanup()







#################################################
#################################################
#################################################
#################################################
#%% BBB

import socket
from collections import deque
import random
import time

try:
    import Adafruit_BBIO.GPIO as GPIO
    inputChannelPin="P8_9"
    outputChannelPin="P8_7"
    GPIO.setup(inputChannelPin, GPIO.IN)
    GPIO.setup(outputChannelPin, GPIO.OUT, initial=GPIO.LOW)
    GPIO.add_event_detect(inputChannelPin, GPIO.RISING)
except:
    print("Is the script really running on Rasberry?")

def OpenAndStoreCodebookToList(filename):
    validMsgsVector =[]
    with open(filename) as f:
        [validMsgsVector.append(line.strip('\n')) for line in f]
    return validMsgsVector

class NanoNodeMT(object):
    def __init__(self, nodeid, buffsize=30, udpPort=37020):
        self.nodeid = nodeid
        self.buffsize = buffsize
        self.buffer = ""
        self.MsgFromOtherNode = False
        self.dq = deque(maxlen=buffsize)
        self.c = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)  # for UDP Socket
        self.c.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        self.c.bind(("", udpPort))
        self.counter=0
        
    def findValidMsgInBuffer(self):
        self.ValidMsgsFound= ""
        for subbuff in (self.buffer[i:i+self.ValidMsgLen] for i in range(len(self.buffer)-self.ValidMsgLen+1)):
            if subbuff in validMsgsVector:
                self.ValidMsgsFound = subbuff
                break
        print(self.ValidMsgsFound)
    
    def logBufferToFile(self):
        try:
            fb=open("logbuffer"+".txt", "a+", encoding="utf-8")
            fb.write(str(self.buffer) + " | " + str(time.ctime())+"\n")
            fb.close()
        except:
            print("Failed to write buffer to file")
    
    def logMsgToFile(self, nodeType):
        try:
            f=open("log"+str(self.nodeid)+".txt", "a+", encoding="utf-8")
            if (nodeType=="Dest"):
                f.write(" ValidMsgsFound: "+str(self.ValidMsgsFound)+" | Buffer : " + str(self.buffer)+"\n")
                print("Successfully logged data")
            elif (nodeType=="Source"):
                f.write(" RandomMsg: "+str(self.SellectRandomlyMessageOfInterest)+" | Buffer : " + str(self.buffer)+"\n")
                print("Successfully logged data")
            else:
                f.write("NodeType not specified to 'Source' or 'Dest'.\n")            
            f.close()
        except:
            print("Failed to write log file. Check if the file is already in use")  
    
    def raiseFlag(self):
        GPIO.output(outputChannelPin, GPIO.HIGH)
        #time.sleep(0.1)
        self.counter+=1
        print(self.counter)
        GPIO.output(outputChannelPin, GPIO.LOW)

    def resetFlag(self):
        GPIO.output(outputChannelPin, GPIO.LOW)
    
    def checkFlag(self):
        if GPIO.event_detected(inputChannelPin):
            self.MsgFromOtherNode=True
            print("Flag up")
    
    def __del__(self):
        self.c.close()

''' define the buffer size of the each nanonode in BITS (its actually not bits tpyed)!!! '''
Prefix = "1000"
bytesToListen = 2
naNode = NanoNodeMT("10101")
validMsgsVector = OpenAndStoreCodebookToList("codebook1.txt")
naNode.SellectRandomlyMessageOfInterest = validMsgsVector[random.randint(0,2031)]
naNode.ValidMsgLen = len(naNode.SellectRandomlyMessageOfInterest)
#%%
while True:
    if naNode.counter>4:
        break
    msg = []
    data, _ = naNode.c.recvfrom(bytesToListen)
    [msg.append("{0:08b}".format(byte)) for byte in data]
    msg = ''.join(msg)
    for bit in msg:
        naNode.dq.appendleft(bit)
        naNode.buffer = ''.join(naNode.dq)
        #naNode.logBufferToFile()
        naNode.checkFlag()
        #naNode.resetFlag()
        
        if naNode.MsgFromOtherNode==True:
            naNode.findValidMsgInBuffer()
            naNode.logMsgToFile("Dest")
            naNode.MsgFromOtherNode=False
        else:
            if naNode.SellectRandomlyMessageOfInterest in naNode.buffer[:naNode.ValidMsgLen]:
                print("success! MSG %s and buffer %s" %(naNode.SellectRandomlyMessageOfInterest,naNode.buffer))
                naNode.raiseFlag()
                naNode.logMsgToFile("Source")

del naNode
GPIO.cleanup()

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

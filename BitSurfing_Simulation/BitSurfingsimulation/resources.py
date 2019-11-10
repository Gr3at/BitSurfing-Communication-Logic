import random
import threading
from collections import deque
import time
from queue import Queue
import matplotlib.pyplot as plt
import networkx as nx
import logging

usleep = lambda x: time.sleep(x/1000000.0)
arrived_msgs=list()

DEBUG = True

def mylog(s):
    if DEBUG:
        print(s)
#%%
# simulation Class
class Simulation(object):
    def __init__(self, networkDimensions=[3,4], prefix="1110"):
        self.Prefix=prefix
        self.maxNumOfNodes=32 # the maximum possible #nodes for 5bit ids scheme
        self.numOfNodes = (networkDimensions[0]*networkDimensions[1]) if (networkDimensions[0]*networkDimensions[1])<=self.maxNumOfNodes else self.maxNumOfNodes
        self.rows, self.columns = [networkDimensions[0], networkDimensions[1]] if self.numOfNodes<32 else [4,8]
        self.IDlist = list()
        
    #setup the network Graph
    def setupNetworkGraph(self):
        self.IDlist = self.__draftNodeIDs()
        G = nx.grid_graph(dim=[self.rows,self.columns])
        self.DG = nx.DiGraph(G)
        for edge in list(self.DG.edges()):
            if edge != tuple(sorted(edge)) or (edge[0][0]==edge[1][0]==self.columns-1):
                self.DG.remove_edge(*edge)
        i=0
        for n in self.DG.nodes():
            self.DG.node[n]['pos'] = n
            self.DG.node[n]['id'] = self.IDlist[i]
            self.DG.node[n]['hasMessage'] = False
            self.DG.node[n]['isGateway'] = True if len(list(self.DG.successors(n)))==0 else False
            i+=1
        for n in self.DG.nodes():
            self.DG.node[n]['Successors'] = [self.DG.node[temp]['id'] for temp in list(self.DG.successors(n))]
            
    def associate_Graph_to_Nodes(self):
        self.node_pos_vals=list(nx.get_node_attributes(self.DG,'pos').values())
        self.node_ids_vals=list(nx.get_node_attributes(self.DG,'id').values())
        self.node_is_gateway_vals=list(nx.get_node_attributes(self.DG,'isGateway').values())
        self.node_successors_vals=list(nx.get_node_attributes(self.DG,'Successors').values())
        
    def startSimulation(self, bits_from_codebook=False, num_of_msgs_to_send=2, total_Bits_to_send=100000):
        Stream_Source = BitStreamSource(bits_from_codebook)
        global IncommingMsgFlag
        IncommingMsgFlag = dict.fromkeys(list(nx.get_node_attributes(self.DG,'id').values()), False)
        
        # get a list only with the nodes which are not gateways
        tmpids=[i for i, j in zip(self.node_ids_vals, self.node_is_gateway_vals) if j==False]
        
        global msgsList
        msgsList=list()
        while len(msgsList) < num_of_msgs_to_send:
            tmpsender=random.choice(tmpids)
            msgsList.append(Packet(sid=tmpsender,payload="{0:02b}".format(random.randint(0,3))))
        logging.basicConfig(level=logging.INFO, filename=f'sim_{bits_from_codebook}_{num_of_msgs_to_send}_{self.rows}_{self.columns}.log', filemode='w')
        
        self.threads = []
        for t in range(self.numOfNodes):
            self.bitStream_que = Queue()
            self.threads.append(
                    ThreadedNode(
                            self.bitStream_que, 
                            self.node_ids_vals[t], 
                            self.node_pos_vals[t], 
                            args=(1,), 
                            GateWayFlag=self.node_is_gateway_vals[t],
                            prefix=self.Prefix))
            self.threads[t].nodeSuccessors = self.node_successors_vals[t]
            self.threads[t].start() # executes the run() method in each node
            time.sleep(0.2)
            
        #Start BroadCasting Bits
        global global_bit_counter
        global_bit_counter=0 #temp
        logging.info("#######################################################")
        logging.info(f"Starting Simulation: use_codebook={bits_from_codebook}, num_of_msgs_to_send={num_of_msgs_to_send}, total_Bits_to_send={total_Bits_to_send}")
        logging.info("-------------------------------------------------------")
        while global_bit_counter < total_Bits_to_send:
            if len(arrived_msgs)>=num_of_msgs_to_send:
                break
            src_msg = Stream_Source.bcastBit()
            for t in self.threads:
                t.queue.put(src_msg)
            if random.randint(0,1)==0:
                usleep(0.1)
            if global_bit_counter % 200000 == 0:
                print(f"\n\n\n\nglobal_bit_counter = {global_bit_counter}\n\n\n\n")
            global_bit_counter+=1
        
        # log Packets
        logging.info(f"           ----- Packets from msgsList = {len(msgsList)} -----           ")
        for packet in msgsList:
            packet.log_packet()
        logging.info("///////////////////////////////////////////////////////")
        logging.info("*******************************************************")
        logging.info(f"         ----- Packets from arrived_msgs = {len(arrived_msgs)} -----         ")
        print(f"         ----- Packets from arrived_msgs = {len(arrived_msgs)} -----         ")
        #global arrived_msgs
        for packet in arrived_msgs:
            packet.log_packet()
        #finalize simulation
        for t in self.threads:
            t.queue.put(None)
            t.join()

        
    def __draftNodeIDs(self):
        counter=0
        ID_List=list()
        while counter<self.numOfNodes:
            id = "{0:05b}".format(counter)
            ID_List.append(id)
            counter+=1
        return(ID_List)

    
    def visualize_network(self):
        node_pos=nx.get_node_attributes(self.DG,'pos')
        node_ids=nx.get_node_attributes(self.DG,'id')
        node_col = ['blue' if len(list(self.DG.successors(node)))>0 else 'green' for node in self.DG.nodes()]
        edge_col = ['red' for edge in self.DG.edges()]
        nx.draw_networkx(self.DG, node_pos,node_color= node_col, node_size=1400, with_labels=False)
        nx.draw_networkx_edges(self.DG, node_pos,edge_color= edge_col)
        nx.draw_networkx_labels(self.DG,node_pos,labels=node_ids)
        plt.axis('off')
        plt.show()
#%%
# BitStream Source Class
class BitStreamSource(object):
    def __init__(self, input_file='codebook2.txt', use_codebook=False):
        self.position = (0,0)
        self.file=input_file
        self.use_codebook=use_codebook
        self.file_memmory=0
    def bcastBit(self):
        if self.use_codebook==False:
            return("{}".format(random.randint(0,1)))
        elif self.use_codebook==True:
            f = open(self.file,'r')
            f.seek(self.file_memmory)
            tmp=f.read(1)
            while tmp not in "10" or tmp=="":
                if tmp == "":
                    f.seek(0,0)
                tmp=f.read(1)
            self.file_memmory=f.tell()
            f.close()
            return("{}".format(tmp))

#%%
# other Classes
class Packet(object):
    def __init__(self,sid,payload):
        self.senderId=sid
        self.path = list()
        self.payload=payload
        self.marked_to_send = False
        self.creationTime=time.time()
        self.creationBit = 0
        self.arrivalBit = int
        self.BitDifference = int
        self.arrivalTime=float
        self.TimeDifference = float
        self.arrived_to_gateway=False
    def log_packet(self):
        logging.info("###################### New Packet #####################")
        logging.info(f"Packet Path={self.path} \ncreationTime={self.creationTime} \narrivalTime={self.arrivalTime} \ncreation_bit={self.creationBit} \narrival_bit={self.arrivalBit} \nBitDifference={self.BitDifference} \nTimeDifference={self.TimeDifference}")
#%%
#threaded BitSurfing Adapter Class
print_lock = threading.Lock() # to ensure that all prints go together per thread

class ThreadedNode(threading.Thread):
    def __init__(self, queue, nodeid, placement, args=(), GateWayFlag=False, prefix="1110", buffsize=30):
        threading.Thread.__init__(self, args=())
        self.nodeid = nodeid
        self.position = placement
        self.queue = queue
        self.sendingMode = False
        self.buffsize = buffsize
        self.isGateWay = GateWayFlag
        self.Prefix=prefix
        self.validMessages = deque(maxlen=1)
        self.buffer = '000000000000000000000000000000' # 30 zeros
        self.nodeSuccessors = []
        self.ValidSuccessorsMsgs = list()
        self.dq = deque(maxlen=buffsize)
        self.foundBit = 0
        
    def run(self):
        global msgsList
        global global_bit_counter
        
        self.NodeMsgs=list()
        for x in msgsList:
            if type(x)==Packet and x.senderId == self.nodeid:
                self.NodeMsgs.append(msgsList.index(x)) # get the indexes of this node messages
        
        print(f"Running NodeID: {self.nodeid}  | Placement: {self.position}")
        while True:
            msg = self.queue.get()
            if msg is None:
                return
            elif int(msg) in range(0,2):
                self.process_message(msg)
                if IncommingMsgFlag[self.nodeid]==True and len(self.validMessages)!=0:
                    #mylog(f"{abs(global_bit_counter - self.foundBit)} |||| global_bit_counter receiver = {global_bit_counter} | from node: {self.nodeid}")
                    if abs(global_bit_counter - self.foundBit)<=50:
                        self.doStuff(self.validMessages[0])
                    else:
                        mylog(f"Faulty flag raise! Msg wasn't likely for this node!")
                        IncommingMsgFlag[self.nodeid]=False
                else:
                    if self.sendingMode==True and self.msgToSentInBuffer():
                        #mylog(f"sending the message now : sender global_bit_counter = {global_bit_counter} | from node: {self.nodeid}")
                        self.raiseNeighborsFlag()
                        self.sendingMode=False
                        self.ValidSuccessorsMsgs=[]
                    elif self.sendingMode==False and len(self.NodeMsgs)!=0:
                        mylog(f"global_bit_counter sender = {global_bit_counter} | from node: {self.nodeid}")
                        nextNodeMsg_idx = self.NodeMsgs.pop()
                        msgsList[nextNodeMsg_idx].path.append(self.nodeid)
                        msgsList[nextNodeMsg_idx].creationBit=global_bit_counter
                        msgsList[nextNodeMsg_idx].creationTime=time.time()
                        msgsList[nextNodeMsg_idx].marked_to_send=True
                        self.sendingMode=True
                        #take extra care
                        self.ValidSuccessorsMsgs = []
                        for successorNode in self.nodeSuccessors:
                            self.ValidSuccessorsMsgs.append(
                                    self.Prefix+msgsList[nextNodeMsg_idx].senderId
                                    +successorNode+msgsList[nextNodeMsg_idx].payload)
            else:
                raise Exception("Error! Unexpected BitStream Input (Not 0 or 1)")
    
    #consider as valid all messages starting with the Prefix (even if the Prefix reappears in the message)
    def process_message(self, message):
        self.dq.appendleft(message)
        self.buffer = ''.join(self.dq)
        if (self.Prefix in self.buffer[14:18]) and (self.nodeid in self.buffer[23:28]):
            self.validMessages.append(self.buffer[len(self.buffer)-16:])
            self.foundBit = global_bit_counter
    
    def doStuff(self, incommingMsg):
        global arrived_msgs
        if self.isGateWay==True:
            #### log Packet ####
            for x in msgsList:
                if type(x)==Packet and x.senderId==incommingMsg[4:9] and x.payload==incommingMsg[14:16] and x.marked_to_send==True and x.arrived_to_gateway==False:
                    x.path.append(self.nodeid)
                    x.arrivalBit=global_bit_counter
                    x.arrivalTime=time.time()
                    x.arrived_to_gateway=True
                    x.BitDifference = x.arrivalBit - x.creationBit
                    x.TimeDifference = round(x.arrivalTime - x.creationTime,2)
                    arrived_msgs.append(x)
                    break
                #mylog(f'{x.senderId} | {x.payload} | {x.marked_to_send} | {x.arrived_to_gateway}')
            ###################
            mylog(f"Reached Gateway! Nodeid: {self.nodeid}")
            IncommingMsgFlag[self.nodeid]=False
        else:
            # if the message is created by this node store for later sending
            if len(self.ValidSuccessorsMsgs)!=0 and self.ValidSuccessorsMsgs[0][4:9]==self.nodeid:
                for x in msgsList:
                    if type(x)==Packet and x.senderId == self.nodeid and x.payload==self.ValidSuccessorsMsgs[0][-2:] and x.marked_to_send==True and x.arrived_to_gateway==False:
                        self.NodeMsgs.append(msgsList.index(x)) # get the indexes of this node messages
            #### log Packet ####
            for x in msgsList:
                if type(x)==Packet and x.senderId==incommingMsg[4:9] and x.payload==incommingMsg[14:16] and x.marked_to_send==True and x.arrived_to_gateway==False:
                    x.path.append(self.nodeid)
                    x.arrivalBit=global_bit_counter
                    x.arrivalTime=time.time()
                    break
            ####################
            mylog(f"Forwarding Msg: {incommingMsg}")
            self.sendingMode=True
            self.ValidSuccessorsMsgs = []
            for successorNode in self.nodeSuccessors:
                self.ValidSuccessorsMsgs.append(self.Prefix+incommingMsg[4:9]+successorNode+incommingMsg[14:16])
            IncommingMsgFlag[self.nodeid]=False
    
    def msgToSentInBuffer(self):
        if len(self.validMessages)==0:
            return False
        else:
            if self.buffer[14:] in self.ValidSuccessorsMsgs:
                mylog("Found the message to send!")
                return True
            else:
                return False
    
    def raiseNeighborsFlag(self):
        for successorNode in self.nodeSuccessors:
            IncommingMsgFlag[successorNode] = True
            mylog(str(self.nodeid) + " --> " + str(successorNode) + " " + str(IncommingMsgFlag[successorNode]) + "\n")

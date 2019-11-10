import random
from collections import deque
import time
from queue import Queue
import matplotlib.pyplot as plt
import networkx as nx
import logging

logging.basicConfig(level=logging.INFO, filename='sim.log', filemode='w')
arrived_msgs=list()
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
        
        self.nodes=[]
        for t in range(self.numOfNodes):
            self.bitStream_que = Queue()
            self.nodes.append(BitSurfingNode(
                            self.bitStream_que, 
                            self.node_ids_vals[t], 
                            self.node_pos_vals[t], 
                            GateWayFlag=self.node_is_gateway_vals[t],
                            prefix=self.Prefix))
            self.nodes[t].nodeSuccessors = self.node_successors_vals[t]
            time.sleep(0.1)
            
        #Start BroadCasting Bits
        global global_bit_counter
        global_bit_counter=0 #temp
        logging.info("#######################################################")
        logging.info(f"Starting Simulation: use_codebook={bits_from_codebook}, num_of_msgs_to_send={num_of_msgs_to_send}, total_Bits_to_send={total_Bits_to_send}")
        logging.info("-------------------------------------------------------")
        while global_bit_counter < total_Bits_to_send:
            if len(arrived_msgs)==num_of_msgs_to_send:
                break
            src_msg = Stream_Source.bcastBit()
            for t in self.nodes:
                t.queue.put(src_msg)
                t.run_next()
            global_bit_counter+=1
        
        # log Packets
        logging.info("           ----- Packets from msgsList -----           ")
        for packet in msgsList:
            packet.log_packet()
        logging.info("///////////////////////////////////////////////////////")
        logging.info("*******************************************************")
        logging.info("         ----- Packets from arrived_msgs -----         ")
        #global arrived_msgs
        for packet in arrived_msgs:
            packet.log_packet()
        #finalize simulation
        for t in self.nodes:
            t.queue.put(None)

        
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
    def __init__(self, input_file='codebook1.txt', use_codebook=False):
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
        self.creationTime=time.time()
        self.creationBit = 0
        self.arrivalBit = int
        self.arrivalTime=float
    def log_packet(self):
        logging.info("#######################################################")
        logging.info("########################New Packet#####################")
        logging.info(f"Packet Path={self.path} \ncreationTime={self.creationTime} \narrivalTime={self.arrivalTime} \ncreation_bit={self.creationBit} \narrival_bit={self.arrivalBit}")
        logging.info("#######################################################")
#%%
#threaded BitSurfing Adapter Class

class BitSurfingNode(object):
    def __init__(self, queue, nodeid, placement, GateWayFlag=False, prefix="1110", buffsize=30):
        self.nodeid = nodeid
        self.position = placement
        self.queue = queue
        self.sendingMode = False
        self.buffsize = buffsize
        self.isGateWay = GateWayFlag
        self.Prefix=prefix
        self.validMessages = deque(maxlen=2)
        self.buffer = '000000000000000000000000000000' # 30 zeros
        self.nodeSuccessors = []
        self.ValidSuccessorsMsgs = list()
        self.dq = deque(maxlen=buffsize)
        self.bitCounter=0
    
        global msgsList
        global global_bit_counter
        
        self.NodeMsgs=list()
        for x in msgsList:
            if type(x)==Packet and x.senderId == self.nodeid:
                self.NodeMsgs.append(msgsList.index(x)) # get the indexes of this node messages
        
    def run_next(self):
        global msgsList
        global global_bit_counter
        msg = self.queue.get()
        if msg is None:
            return
        elif int(msg) in range(0,2):
            self.process_message(msg)
            if IncommingMsgFlag[self.nodeid]==True and len(self.validMessages)!=0:
                print(f"global_bit_counter receiver = {global_bit_counter} | from node: {self.nodeid}")
                '''
                # to change normally
                if 
                self.bitCounter=0
                self.doStuff(self.validMessages[1]) # pick the latest message
                ####################
                '''
                if self.nodeid in self.validMessages[1][9:14]:
                    self.bitCounter=0
                    self.doStuff(self.validMessages[1])
                    '''
                    elif self.nodeid in self.validMessages[0][9:14]:
                        self.bitCounter=0
                        self.doStuff(self.validMessages[0])
                    '''
                else:
                    if self.bitCounter>=150:
                        print(f"Faulty flag raise! Msg wasn't likely for this node!")
                        self.bitCounter=0
                        IncommingMsgFlag[self.nodeid]=False
                self.bitCounter+=1
            else:
                if len(self.NodeMsgs)!=0:
                    print(f"global_bit_counter sender = {global_bit_counter} | from node: {self.nodeid}")
                    nextNodeMsg_idx = self.NodeMsgs.pop()
                    msgsList[nextNodeMsg_idx].path.append(self.nodeid)
                    msgsList[nextNodeMsg_idx].creationBit=global_bit_counter
                    msgsList[nextNodeMsg_idx].creationTime=time.time()
                    self.sendingMode=True
                    #take extra care
                    self.ValidSuccessorsMsgs = []
                    for successorNode in self.nodeSuccessors:
                        self.ValidSuccessorsMsgs.append(
                                self.Prefix+msgsList[nextNodeMsg_idx].senderId
                                +successorNode+msgsList[nextNodeMsg_idx].payload)
                if self.sendingMode==True and self.msgToSentInBuffer():
                    self.raiseNeighborsFlag()
                    self.sendingMode=False
        else:
            raise Exception("Error! Unexpected BitStream Input (Not 0 or 1)")
    '''
    na elegxei stin ousia kathe fora an einai mesa sto buffer to minima ekeini ti stigmi kai oxi paliotera
    def scan_the_buffer(self):
        for i in range(len(self.buffer)-16):
            if self.buffer.find(self.Prefix)
    '''
    #consider as valid all messages starting with the Prefix (even if the Prefix reappears in the message)
    def process_message(self, message):
        self.dq.appendleft(message)
        self.buffer = ''.join(self.dq)
        if (self.Prefix in self.buffer[14:18]) and (self.nodeid in self.buffer[23:28]):
            self.validMessages.append(self.buffer[len(self.buffer)-16:])
    
    def doStuff(self, incommingMsg):
        global arrived_msgs
        print("Got Message: " + incommingMsg + " | " + self.nodeid)
        if self.isGateWay==True:
            #### log Packet ####
            for x in msgsList:
                if type(x)==Packet and x.senderId==incommingMsg[4:9] and x.payload==incommingMsg[14:16]:
                    x.path.append(self.nodeid)
                    x.arrivalBit=global_bit_counter
                    x.arrivalTime=time.time()
                    arrived_msgs.append(x)
            ####################
            print("Reached Gateway!" + "Nodeid: " + self.nodeid)
            IncommingMsgFlag[self.nodeid]=False
        else:
            #### log Packet ####
            for x in msgsList:
                if type(x)==Packet and x.senderId==incommingMsg[4:9] and x.payload==incommingMsg[14:16]:
                    x.path.append(self.nodeid)
                    x.arrivalBit=global_bit_counter
                    x.arrivalTime=time.time()
            ####################
            print("Forwarding Msg: "+ incommingMsg)
            self.sendingMode=True
            self.ValidSuccessorsMsgs = []
            for successorNode in self.nodeSuccessors:
                self.ValidSuccessorsMsgs.append(self.Prefix+incommingMsg[4:9]+successorNode+incommingMsg[14:16])
            print(f"Valid messages to send: {self.ValidSuccessorsMsgs}")
            IncommingMsgFlag[self.nodeid]=False
    
    def msgToSentInBuffer(self):
        if len(self.validMessages)==0:
            return False
        else:
            if self.buffer[14:] in self.ValidSuccessorsMsgs:
                print("Found the message to send!")
                return True
            else:
                return False
    
    def raiseNeighborsFlag(self):
        for successorNode in self.nodeSuccessors:
            IncommingMsgFlag[successorNode] = True
            print(str(self.nodeid) + " --> " + str(successorNode) + " " + str(IncommingMsgFlag[successorNode]) + "\n")

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sat Oct  5 15:37:32 2019
@author: Dimitris
"""

'''
A python simulator for BitSurfing Communication Logic
The user has to specify the dimensionality of the network (up to 32 nodes), 
the utilized prefix, Bitstream Creation Source (random or from codebook), 
the total number of messages sent and the simulation duration in bits.
'''

#import sys, getopt
import os

import resources as res
#%% Init Simulation
sim=res.Simulation(networkDimensions=[4,8], prefix="1110")
sim.setupNetworkGraph()
sim.visualize_network()

IncommingMsgFlag = dict() # that's a global variable here
msgsList=list() # a list to put the messages of all nodes
global_bit_counter=int() # a global sent bit counter

sim.associate_Graph_to_Nodes()
sim.startSimulation(bits_from_codebook=True, 
	num_of_msgs_to_send=30, total_Bits_to_send=700000)

duration = 0.05  # seconds
freq = 1200  # Hz
os.system('play -nq -t alsa synth {} sine {}'.format(duration, freq))
#%% all possible 16bit words starting with prefix "1110"
'''
file = 'codebook2.txt'

with open(file, 'w') as f:
    for i in range(65535):
        tmp="{0:016b}".format(i)
        if tmp[:4]=="1110":
            f.write(f"{tmp}\n")
'''
'''
IncommingMsgFlag = dict() # that's a global variable here
msgsList=list() # a list to put the messages of all nodes
#arrived_msgs=list() # the messages which finally made it
global_bit_counter=int() # a global sent bit counter

def main(argv):
    boolean = ''
    numofmsgs = ''
    try:
        opts, args = getopt.getopt(argv,"hb:n:",["boole=","numberofmsgs="])
    except getopt.GetoptError:
        print('test.py -b <useCodebook> -n <numberofmsgs>')
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print('test.py -b <useCodebook> -n <numberofmsgs>')
            sys.exit()
        elif opt in ("-b", "--boole"):
            boolean = arg
        elif opt in ("-n", "--numberofmsgs"):
            numofmsgs = arg
            
    sim=res.Simulation(networkDimensions=[4,4], prefix="1110")
    sim.setupNetworkGraph()
    sim.visualize_network()
    
    sim.associate_Graph_to_Nodes()
    sim.startSimulation(bits_from_codebook=bool(boolean), num_of_msgs_to_send=int(numofmsgs), total_Bits_to_send=600000)


#%%
if __name__ == "__main__":
   main(sys.argv[1:])
'''

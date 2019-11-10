RasberryBBBImplementation

ChangeLog 28/8/2018

1) Randomly Select Message after previous is sent
2) Use 2+1(parent) threads for the implementation
3) Create separate log files for receiving and sending message, as well as add corresponding time
4) Change file manipulation, open files only once at the beginning and close them at the end of the process (much more efficient : https://stackoverflow.com/questions/11349020/python-file-open-close-every-time-vs-keeping-it-open-until-the-process-is-finished)
5) Garbage to udpGetJustMsgBBB and udpGetJustMsgRPi (ready to test)

TO DO
1) Elapsed time since finding Message in Buffer and until sending the actual GPIO pulse.
2) Clean up code


ChangeLog 7/9/2018

Kickoff Implementation in C

1) BitSurfing Sender is properly set up and works great.
2) Receiving node has already :
	a) socket is set up and receives bytes as it should
	b) OpenAndStoreCodebookToVector() function reads data from file and dynamically stores them to linked list (!!!IMPORTANT!!! Don't forget to free linked list from memmory)

TO DO on Receiving Nodes
1) Implement the GPIO signaling (with threads)
2) Bitwise operations between buffer and messages

ChangeLog 10/11/2019

The code is fully functional but requires cleanup.
The Kernel space "best choice" of functional code for each device (Raspberry Pi, BeagleBoneBlack and Banana Pi) is provided in "fileExchangeV2" folder.
The logic provided in this code has a device reinforcement learning training stage which helps the device adopt to network characteristics.

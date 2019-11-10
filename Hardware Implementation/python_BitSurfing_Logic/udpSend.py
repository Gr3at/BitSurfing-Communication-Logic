import random
import socket
import time

MsgSizeInBytes = 2

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

while True:
	msg = []
	[msg.append(random.randint(0,255)) for byte in range(MsgSizeInBytes)]
	message = bytes(msg)
	s.sendto(message, ('<broadcast>', 37020))  # change to '127.1.0.1' one in deploy mode
	print("message sent!")
	time.sleep(0.01)

s.close()

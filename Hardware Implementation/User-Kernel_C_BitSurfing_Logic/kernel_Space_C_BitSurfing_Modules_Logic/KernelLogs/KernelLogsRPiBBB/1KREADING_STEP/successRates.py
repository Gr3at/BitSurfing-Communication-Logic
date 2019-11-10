recRPi="recRPi.txt"
recBBB="recBBB.txt"
sendRPi="sendRPi.txt"
sendBBB="sendBBB.txt"
files = [recRPi,recBBB,sendBBB,sendRPi]
for filename in files:
	print(filename)
	file = open(filename, "r")
	for line in file:
		if line!='\n':
   			print(line)

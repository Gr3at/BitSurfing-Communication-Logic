'''
a script to identify all the unique sender-receiver BitSurfing combinations existing in a file.
result shown in unique_Ids.png.
'''

file = 'codebook1.txt'
senders_List = list()
receivers_List = list()

# get the unique ids in sender and receiving lists
with open(file) as f:
    for line in f:
        if line[4:9] not in senders_List:
            senders_List.append(line[4:9])
        if line[9:14] not in receivers_List:
            receivers_List.append(line[9:14])

# get the unique pairs of ids
senders_Dict = {}
with open(file) as f:
    for sid in senders_List:
        senders_Dict[sid] = list()
    for line in f:
        if (line[9:14] not in senders_Dict[line[4:9]]) and (line[4:9] != line[9:14]):
            senders_Dict[line[4:9]].append(line[9:14])

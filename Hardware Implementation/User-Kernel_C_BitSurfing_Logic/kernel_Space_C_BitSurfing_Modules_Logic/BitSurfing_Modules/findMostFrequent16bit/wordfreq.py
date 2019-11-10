combs = {}
for i in range(256):
	for j in range(256):
		combs[chr(i)+chr(j)] = 0

print(combs['FP'])

files = ['words.txt', 'wordsa.txt', 'words_alpha.txt', '5kwordlist.txt', '1.txt', '2.txt', '3.txt', '4.txt', '5.txt', '6.txt', '7.txt']

def fileWordsFreq(file, combs):
	prevChar = ' '

	with open(file) as f:
		a="".join(line.strip() for line in f)
	print(a[:40])

	for char in a:
		temp2char=prevChar+char
		combs[temp2char]+=1
		prevChar = char

for file in files:
	fileWordsFreq(file, combs)

# sort data by value and store results to file
sorted_by_value = sorted(combs.items(), key = lambda kv: kv[1])
#print(''.join(sorted_by_value[-1]))

import io
with io.open("freqs2.txt", 'w', encoding='utf-8') as wf:
	for k,v in sorted_by_value[-2000:-1]:
		tmpData = str(k) + '\t' + str(v)# + '\n'
		print(tmpData)
		#tmpData = tmpData.decode('utf-8')
		#wf.write(tmpData)

"""
with open("freqs.txt", "w") as wf:
	for i in range(1800):
		for k,v in sorted_by_value[-i]:
			wf.write('\n'.join('{} {}'.format(str(k),str(v))))
"""

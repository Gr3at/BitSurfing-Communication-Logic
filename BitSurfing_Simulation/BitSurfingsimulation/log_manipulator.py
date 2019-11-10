
import os

files=os.listdir()

for file in files:
    print(f'#############\n{file}')
    with open(file) as f:
        for line in f:
            if 'msgsList' in line:
                print(f'{line}')
            elif 'arrived_msgs' in line:
                print(f'{line}')

#%% get bitDifference

import os

files=os.listdir()

for file in files:
    print(f'#############\n{file}')
    with open(file) as f:
        flag=False
        for line in f:
            if 'arrived_msgs' in line:
                flag=True
            if flag==True and 'BitDifference' in line:
                print(f'{line}')

#%%
def Average(lst): 
    return sum(lst) / len(lst) 

Average(s12) = 34964.4
Average(s22) = 49111.9
Average(s23)=51706.25
Average(s24) = 76219.88
Average(s34)=122698
Average(s44)=104197.85
Average(s45)=72562.5
Average(s46) = 63569.8
Average(s56)=214691.8
Average(s48) = 236926.5

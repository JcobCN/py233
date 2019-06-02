#! python3
# pw. py - An insercure password locker program.

'''
PASSWORDS = {'email': 'asddkjljlkj',
            'blog':'3d342dsf',
            'luggage':'2123'
            }

import sys, pyperclip
if len(sys.argv) < 2:
    print('Usage: py pw. py [account] - copy account password')
    sys.exit()

account = sys.argv[1]

if account in PASSWORDS:
    pyperclip.copy(PASSWORDS[account])
    print('Password for' + account + 'copied to clipboard.')
else:
    print('There is no acctount named ' + account)
'''

# Adds bullet points to the start 
# of each line of test on the clipboard

'''
import pyperclip

text = pyperclip.paste()

lines = text.split('\n')
for i in range(len(lines)):
   lines[i] = '* ' + lines[i] 

text = '\n'.join(lines)
pyperclip.copy(text)
'''
tableData = [['apples', 'oranges', 'cherries', 'banana'],
['Alice', 'Bob', 'Carol', 'David'],
['dogs', 'cats', 'moose', 'goose']]

def printTable(str1:str):
    strtemp = ""
    maxlen = 0
    for j in range(len(tableData[0])):
        for i in range(len(tableData)):
            strtemp = strtemp + ' ' + str1[i][j]
            if len(str1[i][j]) > maxlen:
                maxlen = len(str1[i][j])
        print(strtemp.rjust(maxlen))
        strtemp = ''

printTable(tableData)
#! python3

import pyperclip, shelve
import sys

mb = shelve.open('mulclib')

if len(sys.argv) == 3 and sys.argv[1] == 'save':
    mb[sys.argv[2]] = pyperclip.paste()
elif len(sys.argv) == 3 and sys.argv[1] == 'delete':
    del mb[sys.argv[2]]
else: #len(sys.argv) == 2:
    keyList = list(mb.keys())
    print(keyList)
    # for keyList in range(mb.keys()):
    #     print('keyList=%d\n'%(keyList))
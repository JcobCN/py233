#! python3
import re, pyperclip

phoneRegex = re.compile(r'''(
    (\d{3}|\(\d{3}\))?
    (\s|-|\.)?
    (\d{3})
    (\|-|\.)
    (\d{4})
    (\s*(ext|x|ext.)\s*(\d{2,5}))?
)''', re.VERBOSE)

emailRegex = re.compile(r'''(
    [a-zA-Z0-9._%+-]+ #username
    @   # @symbol
    [a-zA-Z0-9.-]+ #domain name
    (\.[a-zA-Z]{2,4})  # dot-something
)''', re.VERBOSE)

text = str(pyperclip.paste())
matches = []
for groups in phoneRegex.findall(text):
    print('groups:'+ str(groups))
    phoneNum = '-'.join([groups[1], groups[3], groups[5]])
    if groups[8] != '':
        phoneNum += ' x' + groups[8]
    matches.append(phoneNum)
    print('matches: ' + str(matches))
for groups in emailRegex.findall(text):    
    print('groups:'+ str(groups))
    matches.append(groups[0])
    print('matches: ' + str(matches))


import pyperclip

clipData = pyperclip.paste()
print(clipData)
clipData = clipData.split('\n')
print(clipData)

# str1:str
str1 = ''

for i in range(len(clipData)):
    str1 = str1 + ('[%d] = "%s", &%s,\n' % (i, clipData[i], clipData[i]) )

pyperclip.copy(str1) 
b = pyperclip.paste()
# print(str1)
# print(b)
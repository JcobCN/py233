#! python3
import re

def fuc1(*word):
    if len(word) > 1:
        str1 = str(word[0])
        str2 = str(word[1])
        wordRegex = re.compile(str2)
        str3 = wordRegex.sub('', str1)
        print(str3)
    else:
        wordRegex = re.compile(r'^\s*\s*$')
        mo = wordRegex.search(str(word[0]))
        print(mo.groups())

fuc1('   asdHHHHJJJ  \n2222   ')

# def func2(*word:str):
#     print(type(word))
#     for n in word:
#         print(type(n))
#         print('%s, %s' % (n, type(n)) )
# func2('2', '3', '4')
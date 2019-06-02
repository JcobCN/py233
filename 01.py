import random, sys, os, math
# from random import randint
# from random import * 等同于using namespace 将所有函数包含进来

for num in range(5, -1, -1):
    print(random.randint(1, 10))

def hello():
    print('helo 111 %d'% (2333))
    print('helo 222','^^^' , sep='%')
    print('helo 333', '233', sep='#')

#exception
def spam(divideBY):
    try:
        return 42 / divideBY
    except ZeroDivisionError:
        print('Error: Invalid argument.  ', end='')

print(spam(2))
print(spam(12))
print(spam(0))
print(spam(1)) 
#end exception

import collatz

while True:
    try:
        num = int(input())
    except ValueError:
        print('error value.')
        continue
    else:
        break
        
while True:
    num = collatz.collatz(num)
    if num == 1:
        break
    
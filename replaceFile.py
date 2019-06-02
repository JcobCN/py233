import os

y = open('233','r')
y_new = open('233_new','w')
tihuan = input('输入要替换的替换：')
tihuan_new = input('输入新内容：')
tihuan_new.encode()

for i in y: #迭代旧文件
    print('for:' + i)
    if tihuan in i: #查找每一行是否存在 替换字符串
        i = i.replace(tihuan,tihuan_new) #字符串替换
    y_new.write(i)

y.close()
y_new.close()

os.renames('233_new', '233_2') #重命名，将旧文件重命名成新文件

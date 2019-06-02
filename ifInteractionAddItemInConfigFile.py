import os
import chardet
import re

def getConfigFileDir(floderName:str, configDir:list):

    dirList = os.listdir(floderName) #列出目录
    for i in dirList:#迭代子目录名字
        path = floderName+'\\'+i    #组合子目录地址

        if os.path.isdir(path) is True: #若是目录继续列出子目录
            subDirList = os.listdir(floderName+'\\'+i)
            # print(i+':'+str(subDirList))
            for j in subDirList: #查找子目录中的configFiles文件夹
                if j == 'configFiles' : #找到文件夹，返回路径
                    configDir.append(floderName+'\\'+i+'\\'+j)    #组合config文件夹绝对地址，放进数组



def appendItemInConfig(fp, filePath:str, oldStr:str, appendStr:str):
    print(filePath+'_bak')
    newFp = open(filePath+'_bak', 'w')

    fp.seek(0, os.SEEK_SET) #迭代旧文件内容
    for i in fp:
        mo = re.match(oldStr, i)
        if mo != None: #找到就将需要的项追加
            print(mo.group(0))
            i = mo.group(0)+','+appendStr
            print(i)
            print()
        newFp.write(i)
    newFp.close()


'''
如果文件中含有needle项，则执行相关操作
'''
def haveNeedleInFileAndAppendItem(path:str, needle:str):
    rf = open(path, 'rb') #检测文件编码类型，需要用二进制读取
    data = rf.read(2000)    #读取一定字节的数据，进行判断
    code = chardet.detect(data) #判断编码格式
    rf.close()

     #得到编码格式后重新打开
    encoding = ''
    if code['encoding'].upper() != 'UTF-8': #若判断不是utf8，则全部用GBK编码打开
        encoding = 'GBK'
    else:
        encoding = code['encoding']

    #迭代文件内容
    found = 0
    rf = open(path, 'r', encoding= encoding)
    for i in rf:
        mo = re.match(needle, i, re.I)
        if mo != None:
            print(mo.group())
            print('找到')
            found = 1;
            break;

    done = 0
    if found == 1:
        appendItemInConfig(rf, path, r'^SEMFunctionSystemAppSetup.*', 'Videosource\n')
        done = 1

    rf.close()

    if done :
        #替换完成，删除源文件，rename _bak文件
        os.remove(path)
        os.renames(path+'_bak', path)





if __name__ == '__main__':
    path = os.path.join('E:/', 'project', 'trunk_8.1', 'project')
    print(path)
    os.chdir(path)
    print('当前工作目录：'+os.getcwd())
    print( os.listdir(path) )

    configDirList:list = []
    getConfigFileDir(os.getcwd(), configDirList) #获取配置存放路径列表

    for i in configDirList:
        print(i)
        for file in os.listdir(i): #列出配置文件夹中的文件名
            filePath = i+'\\'+file #组合配置文件绝对地址
            if '_bak' in file: #不要递归的改写_bak配置文件
                continue

            if os.path.isfile(filePath): #是文件则执行
                print(file)
                haveNeedleInFileAndAppendItem(filePath, r'interaEnable\s*Enable')


    # flag = haveNeedleInFile('233', 'interaEnable\tEnable')

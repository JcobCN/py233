import os,re

def findConfigAppItemCount(path:str):
    if os.path.isdir(path) == False:
        print('path=%s is not path'%(path))
        return
    configList = os.listdir(path)
    # print(configList)
    for filename in configList:
        if 'config_function.' in filename:
            # print(filename)
            fileObj = open(path+filename)
            fileObj.writelines
            for lineStr in fileObj:
                # print(lineStr)
                lineStrRegex = re.compile(r'^FunctionSystemAppSetup\s*((\w|,)*)')
                mo = lineStrRegex.search(lineStr)

                if mo != None:
                    itemList = mo.group(1).split(',')
                    if(len(itemList) <= 10):
                        print('Filename=**%s** $1=%s len=%d'% ( filename, mo.group(1), len( itemList ) ) )
print('3531:')
findConfigAppItemCount(r'E:\project\trunk_8.1_bugfix\project\a4smain\configFiles\\')
print('3531a:')
findConfigAppItemCount(r'E:\project\trunk_8.1_bugfix\project\hx_main\configFiles\\')
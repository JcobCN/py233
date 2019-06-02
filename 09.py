
'''
Directory tree
'''
def tx1():
    import os
    for folderName, subfolders, filenames in os.walk(r'E:\my_program\py'):
        print('folderName %s' % (folderName))
        print('subfolders %s' % (subfolders))
        print('filenames %s' % (filenames))
    
def tx2():
    import os, zipfile
    os.chdir('e:/my_program/py')
    zipObj = zipfile.ZipFile('绝地源码.zip')
    for name in zipObj.namelist():
        print(name)
    # zipObj.extractall()
    zipObj.close()

tx2()
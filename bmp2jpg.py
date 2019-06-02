import os
import re
from PIL import Image
'''
遍历目录
将.bmp转为1080pjpg
'''

dstPath = 'C:/Users/tianjiang/Pictures/233/'
fileList = os.listdir(dstPath)
i=1
for picFile in fileList:
    if '.bmp' in picFile:
        mo = re.search(r'.*(\d{4}-\d{1,2}-\d{1,2}).*\.bmp', picFile)
        infile = dstPath+mo.group(0)
        outfile = dstPath+'%02d.jpg'%(i)
        if infile != outfile:
            Image.open(infile).resize((1920, 1080)).save(outfile)
            i+=1

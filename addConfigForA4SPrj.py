import os,sys

def addConfig(filename:str, needleStr:str, addStr:str):
    filePath = os.path.abspath(filename);


if __name__ == '__main__':

    if(len(sys.argv) < 2):
        print("argument too less %d" % (len(sys.argv)))
    else:
        addConfig(sys.argv[1], sys.argv[2], sys.argv[3])
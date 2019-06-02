import sys
import socket
import time
import struct

def checksum(data):
    n = len(data)
    m = n % 2
    sum = 0

    print(type(data), n)
    print(data[0], data[1])

    for i in range(0, n-m, 2):
        sum += (data[i]) + ( (data[i+1]) << 8 )
        print('%x+%x, sum=%#x'% (data[i], data[i+1]<<8, sum) )

    if m:
        sum += (data[-1])

    sum = (sum >> 16) + (sum & 0xffff)
    sum += (sum >> 16)
    answer = ~sum & 0xffff
    # 主机字节序转网络字节序列（参考小端序转大端序）
    answer = answer >> 8 | (answer << 8 & 0xff00)
    print( '%#x'% (answer ) )


def requsetPing(data_type,
                data_code, data_checksum,
                data_ID, data_sequence, data_content):

    #>表示大端格式， 网络用大端格式
    icmp_packet = struct.pack('>BBHHH32s', data_type, data_code, data_checksum, data_ID,
                              data_sequence, data_content)
    print(icmp_packet)
    icmp_checksum = checksum(icmp_packet)#获取校验和
    icmp_packet = struct.pack('>BBHHH32s', data_type, data_code, data_checksum, data_ID,
                              data_sequence, data_content)

    # rawsocket = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.getprotobyname('icmp'))


def ping(host):
    data_type = 8 #ICMP Echo Requet ICMP Type 8bit
    data_code = 0 #must be zero  ICMP code 8bit
    data_checksum = 0 # 校验和 16bit
    data_ID = 0 #identifier 16bit
    data_sequence = 1 #序列号 16bit
    # data_content = b'fxckavafxckavafxckavafxckavaqnmb' # 填充内容(选) 32bit~更多
    data_content = b'abcdefghijklmnopqrstuvwabcdefghi'

    requsetPing(data_type,
                data_code, data_checksum,
                data_ID, data_sequence, data_content)
    return

    dst_addr = socket.gethostbyname(host) #将主机名转换为ipv4地址格式
    print("正在 Ping {0} [{1}] 具有 32 字节的数据:".format(host, dst_addr))

    # for i in range(0,4):


if __name__ == '__main__':
    ping('www.baidu.com')

    # if len(sys.argv) == 2:
    #     ping(sys.argv[1])
    # else:
        # print('without host input! Error!')
        # a = input("press any key and exit!")

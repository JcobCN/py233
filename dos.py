from ping import *
import os
from ctypes import *


# 发送 数据包
def send_packet(addr, packet):
    rawsocket = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.getprotobyname('icmp'))

    #   使socket 不封装ip数据报, 需要管理员权限才能运行。 linux需要root， windows需要管理员权限
    rawsocket.setsockopt(socket.SOL_IP, socket.IP_HDRINCL, 1)


    # send data to socket
    ret = rawsocket.sendto(packet, (addr, 80))  # sendto 第二个参数必须是tuple类型， 一般是add,port, icmp port参数无用
    print( 'ret={0}, addr={1}'.format(ret, addr) )
    return rawsocket


#  构造ip报文协议头 用ctype 定义 c语言结构体
class IP(Structure):
    _fields_ = [
        ("hl",             c_ubyte, 4),  # ('名称', 类型, 位域(optional))
        ("v",         c_ubyte, 4),      # hl, v共用一个 byte， 注意ip报文协议 version在前，hl在后。 所以v在高位（应该是小端），调整下结构体两个变量的顺序
        ("tos",             c_ubyte),
        ("len",             c_ushort),
        ("id",              c_ushort),
        ("off",          c_ushort),
        ("ttl",             c_ubyte),
        ("p",    c_ubyte),
        ("sum",             c_ushort),
        ("src",             c_uint),
        ("dst",             c_uint),
    ]


def test():
    tmp_packet = b'\x45\x00\x00\x3c\x3c\xb2\x00\x00\x40\x01\xaf\x63\xc0\xa8\x0b\x5a\xc0\xa8\x02\x01\x08\x00\x4c\xfb\x00\x01\x00\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f\x70\x71\x72\x73\x74\x75\x76\x77\x61\x62\x63\x64\x65\x66\x67\x68\x69'
    print('tmp_packet:', tmp_packet)
    print('tmp_packet2:', tmp_packet.hex())

if __name__ == '__main__':

    data_type = 8  # ICMP Echo Requet ICMP Type 8bit
    data_code = 0  # must be zero  ICMP code 8bit
    data_checksum = 0  # 校验和 16bit
    data_ID = 0  # identifier 16bit
    data_sequence = 1  # 序列号 16bit
    # data_content = b'fxckavafxckavafxckavafxckavaqnmb' # 填充内容(选) 32bit~更多
    data_content = b'abcdefghijklmnopqrstuvwabcdefghi'

    icmp_packet = requsetPing(data_type,  # 得到ICMP封包
                              data_code, data_checksum,
                              data_ID, data_sequence, data_content)

    fakeip = '192.168.11.90'
    dstip = '192.168.2.1'

    #   给ip头信息赋值
    ip_packet = IP()
    ip_packet.v = 4
    ip_packet.hl = 5
    ip_packet.tos = 0
    ip_packet.len = ip_packet.hl*4 + len(icmp_packet)
    ip_packet.id = os.getpid()
    ip_packet.off = 0
    ip_packet.ttl = 64
    ip_packet.p = socket.IPPROTO_ICMP
    ip_packet.sum = 0
    ip_packet.src = int.from_bytes( socket.inet_aton(fakeip) , 'little')
    ip_packet.dst = int.from_bytes(socket.inet_aton(dstip) , 'little')

    #  通过string_at 返回 二进制流
    ip_bytes:bytes = string_at(addressof(ip_packet), sizeof(ip_packet))

    #  打包ip报文和icmp报文头
    socket_packet = ip_bytes+icmp_packet
    print('packet:', socket_packet)
    print('packet2:', socket_packet.hex())

    #目标地址
    dst_addr = socket.gethostbyname(fakeip)



    while True:
        #  发送
        sock = send_packet(dst_addr, socket_packet)
        time.sleep(0.5)


'''
    这种方式没办法对位进行精确赋值。
    
    后来想一下，小于byte的位，用 & | <<  等操作符，还是可行的。就是不太方便
    
    # ip包头封装
    ip_v = 4  # ip version 4
    ip_hl = 5  # ip 一行32bit 4byte 一般为@5行@，省去了ip数据包的扩展
    ip_tos = 0  # 服务类型，区分服务，没有用过。 type of service / ip.dsfield
    ip_len = ip_hl * 4 + len(icmp_packet)  # ip数据包，ip头部长度+数据长度
    ip_id = os.getpid()  # id
    ip_off = 0  # offset
    ip_ttl = 64  # ttl
    ip_p = socket.IPPROTO_ICMP
    ip_checksum = 0  # 首部校验和
    ip_srcaddr = socket.inet_aton('192.168.11.2')
    ip_dstaddr = socket.inet_aton('192.168.11.90')
    print(ip_dstaddr)
'''


from ping import *
import os


# 发送 数据包
def send_packet(addr, packet):
    rawsocket = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.getprotobyname('icmp'))
    rawsocket.setsockopt(socket.SOL_IP, socket.IP_HDRINCL)

    ip_packet = struct.pack('>BBHHH32s', data_type, data_code, data_checksum, data_ID,
                            data_sequence, data_content)

    # send data to socket
    rawsocket.sendto(packet, (addr, 80))  # sendto 第二个参数必须是tuple类型， 一般是add,port, icmp port参数无用
    return rawsocket


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
    ip_srcaddr = socket.inet_aton('192.168.2.244')
    ip_dstaddr = socket.inet_aton('')


    dst_addr = socket.gethostbyname('192.168.11.90')  # 将主机名转换为ipv4地址格式
    while True:
        sock = send_packet(dst_addr, icmp_packet)

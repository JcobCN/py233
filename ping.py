import sys
import socket
import time
import struct
import select

def checksum(data):
    n = len(data)
    m = n % 2
    sum = 0

    for i in range(0, n-m, 2):
        sum += (data[i]) + ( (data[i+1]) << 8 )
        # print('%#x+%#x, sum=%#x'% (data[i], data[i+1]<<8, sum) )

    if m:
        sum += (data[-1]) #剩一个单数，则直接加

    sum = (sum >> 16) + (sum & 0xffff)
    sum += (sum >> 16)
    answer = ~sum & 0xffff
    # 主机字节序转网络字节序列（参考小端序转大端序）
    answer = answer >> 8 | (answer << 8 & 0xff00)
    print( '%#x'% (answer ) )
    return answer


def requsetPing(data_type,
                data_code, data_checksum,
                data_ID, data_sequence, data_content):

    #>表示大端格式， 网络用大端格式
    icmp_packet = struct.pack('>BBHHH32s', data_type, data_code, data_checksum, data_ID,
                              data_sequence, data_content)
    print(icmp_packet)
    icmp_checksum = checksum(icmp_packet)#获取校验和

    #再将icmp_checksum 加入，再计算校验和
    icmp_packet = struct.pack('>BBHHH32s', data_type, data_code, icmp_checksum, data_ID,
                              data_sequence, data_content)

    return icmp_packet


#发送 icmp数据包
def send_icmp_packet(addr, packet):
    rawsocket = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.getprotobyname('icmp'))

    #send data to socket
    rawsocket.sendto(packet, (addr, 80)) #sendto 第二个参数必须是tuple类型， 一般是add,port, icmp port参数无用
    return rawsocket



def reply_ping(send_time, rawsocket, data_squence, timeout = 2):
    while True:
        started_select = time.time()
        what_ready = select.select([rawsocket], [], [], timeout)

        wait_for_time = ( time.time() - started_select )#计算出时间差
        if what_ready[0] == []: # Timeout
            return -1

        time_received = time.time()
        received_packet, addr = rawsocket.recvfrom(1024)

        print(received_packet)
        icmpHeader = received_packet[20:28]

        type, code, checksum, packet_id, squence = struct.unpack(">BBHHH",
                                                                 icmpHeader)

        if type == 0 and squence == data_squence:
            return time_received - send_time

        timeout = timeout - wait_for_time # 减去等待时间
        if timeout <= 0 :
            return -1

def ping(host):
    data_type = 8 #ICMP Echo Requet ICMP Type 8bit
    data_code = 0 #must be zero  ICMP code 8bit
    data_checksum = 0 # 校验和 16bit
    data_ID = 0 #identifier 16bit
    data_sequence = 1 #序列号 16bit
    # data_content = b'fxckavafxckavafxckavafxckavaqnmb' # 填充内容(选) 32bit~更多
    data_content = b'abcdefghijklmnopqrstuvwabcdefghi'

    icmp_packet = requsetPing(data_type, #得到ICMP封包
                              data_code, data_checksum,
                              data_ID, data_sequence, data_content)



    dst_addr = socket.gethostbyname(host) #将主机名转换为ipv4地址格式
    rawsocket = send_icmp_packet(dst_addr, icmp_packet)
    send_request_ping_time = time.time()

    times = reply_ping(send_request_ping_time, rawsocket, data_sequence)

    if times > 0:
        print("正在 Ping {0} [{1}] 具有 32 字节的数据, 时间={2}ms:".format(host, dst_addr, int(times*1000)))
        time.sleep(0.7)
    else:
        print('请求超时。%s'% (dst_addr))

    # for i in range(0,4):


if __name__ == '__main__':
    ping('www.baidu.com')

    # if len(sys.argv) == 2:
    #     ping(sys.argv[1])
    # else:
        # print('without host input! Error!')
        # a = input("press any key and exit!")

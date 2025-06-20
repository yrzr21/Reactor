import socket
import struct
import time
import threading

SERVER_ADDR = ('192.168.58.128', 5005)
MESSAGE = b'ping' * 10  # 40B
NUM_REQUESTS = 1000000
NUM_THREADS = 30 # 并发数

results = []

def encode_message(message: bytes) -> bytes:
    return struct.pack('!I', len(message)) + message  # 4B报文头 + 数据

def decode_message(sock: socket.socket) -> bytes:
    # 读取4字节长度
    raw_len = sock.recv(4)
    if not raw_len:
        return b''
    msg_len = struct.unpack('!I', raw_len)[0]
    # 读取正文
    data = b''
    while len(data) < msg_len:
        chunk = sock.recv(msg_len - len(data))
        if not chunk:
            break
        data += chunk
    return data

def worker(thread_id: int):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(SERVER_ADDR)
    encoded = encode_message(MESSAGE)

    latencies = []
    for _ in range(NUM_REQUESTS):
        start = time.time()
        try:
            s.sendall(encoded)
        except BrokenPipeError:
            print("连接已关闭，发送失败")
            return
        s.sendall(encoded)
        _ = decode_message(s)
        end = time.time()
        latencies.append((end - start) * 1000)  # 毫秒

    s.close()
    results.append(latencies)

def run_test():
    threads = []
    start_time = time.time()

    for i in range(NUM_THREADS):
        t = threading.Thread(target=worker, args=(i,))
        t.start()
        threads.append(t)

    for t in threads:
        t.join()

    end_time = time.time()
    duration = end_time - start_time
    total_requests = NUM_REQUESTS * NUM_THREADS
    total_bytes = len(MESSAGE) * total_requests * 2  # 请求 + 回复

    print(f'总耗时: {duration:.2f}s')
    print(f'QPS: {total_requests / duration:.2f} req/s')
    print(f'Throughput: {total_bytes / (1024 * 1024) / duration:.2f} MB/s')

    # 统计延迟
    all_latencies = [lat for batch in results for lat in batch]
    if not all_latencies:
        print("没有任何请求成功响应，检查服务端日志")
        return
    avg_latency = sum(all_latencies) / len(all_latencies)
    p99_latency = sorted(all_latencies)[int(0.99 * len(all_latencies))]

    print(f'平均延迟: {avg_latency:.2f} ms')
    print(f'P99 延迟: {p99_latency:.2f} ms')

if __name__ == '__main__':
    run_test()

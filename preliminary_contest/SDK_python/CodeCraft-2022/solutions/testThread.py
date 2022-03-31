import csv
import copy
import math
import time

import threading
calculate_size = 10000
def thread_average_max_spread(start_index, end_index, get_lock, release_lock, output):
    print("thread start: ", start_index)
    start = time.time()
    for t in range(start_index, end_index):
        sum = 0
        for i in range(calculate_size):
            sum += (87767 - i)
    end = time.time()
    print("thread ", start_index, " calculate time = ", end - start)
    if not get_lock:
        wstart = time.time()
        output.write("thread " + str(start_index) +"\n")
        for t in range(start_index, end_index):
            for i in range(35):
                output.write("index = " + str(i) + "Hello !")
            output.write("\n")
        release_lock.release()
        wend = time.time()
        print("thread ",start_index, "  write time = ",wend - wstart)
    else:
        get_lock.acquire()
        wstart = time.time()
        output.write("thread " + str(start_index) +"\n")
        for t in range(start_index, end_index):
            for i in range(35):
                output.write("index = " + str(i) + "Hello !")
            output.write("\n")

        release_lock.release()
        wend = time.time()
        print("thread ", start_index, " write time = ", wend - wstart)
        

def compare_single_thread(output):
    start_time = time.time()
    for t in range(timestamps):
        sum = 0
        for i in range(calculate_size):
            sum += (87767 - i)
    end_time = time.time()
    print("single thread calculate time = ", end_time - start_time)

    wstart = time.time()
    output.write("single thread" + str(timestamps) + "\n")
    for t in range(timestamps):
        for i in range(35):
            output.write("index= " + str(i) + "Hello!")
        output.write("\n")
    wend = time.time()
    print("single thread write time = ", wend - wstart)

lock_1 = threading.Lock()
lock_2 = threading.Lock()
lock_3 = threading.Lock()
lock_4 = threading.Lock()
lock_5 = threading.Lock()
timestamps = 10000
if __name__=='__main__':
    solution = open("output/testThread2.txt",mode='w')
    compare_single_thread(solution)
'''
    run_st = time.time()
    
    lock_1.acquire()
    lock_2.acquire()
    lock_3.acquire()
    lock_4.acquire()
    lock_5.acquire()
    step = math.ceil(timestamps/5)
    
    th1 = threading.Thread(target=thread_average_max_spread, args=(0, step, None, lock_1, solution))
    th2 = threading.Thread(target=thread_average_max_spread, args=(step, step*2, lock_1, lock_2, solution))
    th3 = threading.Thread(target=thread_average_max_spread, args=(step*2, step*3, lock_2, lock_3, solution))
    th4 = threading.Thread(target=thread_average_max_spread, args=(step*3, step*4, lock_3, lock_4, solution))
    th5 = threading.Thread(target=thread_average_max_spread, args=(step*4, timestamps, lock_4, lock_5, solution))

    th1.start()
    th2.start()
    th3.start()
    th4.start()
    th5.start()

    lock_5.acquire()
    solution.close()

    run_ed = time.time()
    print("run time =", run_ed - run_st)
'''
import os
import time
from random import randint

def get_temp():
    with open('/sys/class/thermal/thermal_zone0/temp','r') as t:
        return int(t.read())

def gen_prc_nums():
    l = [randint(0,5) for _ in range(4)]
    for num in l:
        if num > 0:
            return l
        i = randint(0,3)
        l[i] += 1
        return l

def list_to_command(lst):
    s = "stress"
    if lst[0]:
        s += " -c "+str(lst[0])
    if lst[1]:
        s += " -d "+str(lst[1])
    if lst[2]:
        s += " -i "+str(lst[2])
    if lst[3]:
        s += " -m "+str(lst[3])
    s += " -t 120"
    return s

def wait_temp_drop(sleep_time=5, sleep_temp=55000):
    cur_temp = get_temp()
    print("CURRENT TEMP: %d" % cur_temp)
    while cur_temp > sleep_temp:
        time.sleep(sleep_time)
        cur_temp = get_temp()
        print("CURRENT TEMP: %d" % cur_temp)


if __name__ == '__main__':
    num_runs = 100
    cur_run = 1
    commands = [list_to_command(gen_prc_nums()) for _ in range(num_runs)]
    c_dict = {cmd: 1 for cmd in commands}
    commands = c_dict.keys() 

    for cmd in commands:
        print(cmd)

    fl = os.listdir('./data')
    
    for command in commands:
        cf = command.split()
        cf = ''.join(cf)
        cf += '.csv'

        if cf in fl:
            print("skipping in directory file: %s", cf)
            continue

        print("\n\nRUN: %d" % cur_run)
        print("running: %s" % command)
        wait_temp_drop()
        start_time = time.time()
        os.system("./pa "+command)
        run_time = time.time()-start_time
        print('\n\nRan: %s' % command)
        print("\n\nPYTHON SEES RUNTIME: %d\n" % run_time)
        cur_run += 1

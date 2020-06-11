import pandas as pd
import numpy as np  
import random as random


class DataStream:

    def __init__(self, filename):
        self.df = pd.read_csv(filename)
        self.num = len(self.df)

    def rand_df(self, size):
        series_list = [self.pull_sample() for _ in range(size)]
        return pd.DataFrame(series_list)

    def pull_sample(self):
        frame = self.random_series()
        return self.consolidate(frame)

    def random_series(self):
        start = random.randint(0,self.num-1)
        end = random.randint(start+1,self.num)
        return self.df[start:end]

    def consolidate(self, series):
        time_delta = len(series)
        start_temp = float(series.iloc[0].tempature)/1000
        end_temp = float(series.iloc[-1].tempature)/1000
        temp_delta = (end_temp - start_temp)/time_delta
        #print("start %f end %f time %d" % (start_temp, end_temp, time_delta))
        #print('del temp', temp_delta)
        series = series.drop('tempature', axis=1)
        summed = series.sum(axis=0).astype(float)
        summed = summed/time_delta
        temp0_s = pd.Series(start_temp, ['temp0'])
        d_time_s = pd.Series(float(time_delta), ['del_time'])
        d_temp_s = pd.Series(temp_delta, ['del_temp'])
        summed = summed.append([temp0_s, d_time_s, d_temp_s])
        return summed

    def create_test_data(self):
        temp0 = self.df.tempature.astype(float)/1000
        temp0.name = 'temp0'
        ones = np.ones(len(temp0))
        del_time = pd.Series(ones.T)
        del_time.name = 'del_time'
        data = self.df.drop('tempature', axis=1)
        data = pd.concat([data,temp0, del_time],axis=1)
        return data


if __name__ == '__main__':
    data = DataStream('c4t120.csv')
    #print(data.df.columns)
    #print(data.rand_df(1).columns)
    x = data.create_test_data()
    print(x.columns)



'''
Index(['tempature', 'PERF_COUNT_HW_CPU_CYCLES', 'PERF_COUNT_HW_INSTRUCTIONS',
       'PERF_COUNT_HW_CACHE_MISSES', 'PERF_COUNT_HW_BRANCH_MISSES',
       'PERF_COUNT_HW_BRANCH_INSTRUCTIONS', 'PERF_COUNT_HW_CACHE_LL_read_miss',
       'PERF_COUNT_HW_CACHE_LL_write_miss'],
      dtype='object')
Index(['PERF_COUNT_HW_CPU_CYCLES', 'PERF_COUNT_HW_INSTRUCTIONS',
       'PERF_COUNT_HW_CACHE_MISSES', 'PERF_COUNT_HW_BRANCH_MISSES',
       'PERF_COUNT_HW_BRANCH_INSTRUCTIONS', 'PERF_COUNT_HW_CACHE_LL_read_miss',
       'PERF_COUNT_HW_CACHE_LL_write_miss', 'temp0', 'del_time', 'del_temp'],
      dtype='object')

'''
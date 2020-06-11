import os
import numpy as np
import pandas as pd
import random

class dataset:

    def __init__(self, data_path=os.path.join('..','data_k'), seed=False):

        if seed:
            random.seed(seed)


        # location of datafiles
        self.data_path = data_path

        # load and shuffle datafiles
        self.train_files = None # list of training file names
        self.test_files = None # list of testing file names
        self.train_test_file_names()

        # for holding dataset dataframs
        self.training_data = None # train/validation   
        self.test_data = None # unwindowed 

    def train_test_file_names(self, test_split=.2):
        # get and shuffle files
        files = os.listdir(self.data_path)
        files = [os.path.join(self.data_path, f) for f in files]
        random.shuffle(files)
        # create splits
        split_idx = len(files) - int(len(files)*test_split)
        self.train_files = files[:split_idx]
        self.test_files = files[split_idx:]

    def build_train_set(self):
        files = self.train_files
        set_dfs = [dataset._sample(file) for file in files]
        df = pd.concat(set_dfs)
        self.training_data = df

    def build_test_set(self):
        files = self.test_files
        set_dfs = [dataset._delta_temp_column(file) for file in files]
        df = pd.concat(set_dfs)
        self.test_data = df

    @staticmethod
    def _delta_temp_column(file):
        df = dataset.load_dataframe(file)
        t0 = df.temp
        t1 = t0.iloc[1:].to_numpy()
        t0 = t0.iloc[:-1].to_numpy()
        dt = t1-t0
        df = df[:-1]
        df['d_temp'] = dt
        return df


    @staticmethod
    def load_dataframe(filename):
        # load and organize columns
        #print(filename)
        df = pd.read_csv(filename)
        df.temp = df.temp/1000
        #df = df.drop('')
        return df


    @staticmethod 
    def _sample(filename, num_samples=100, min_window=4, max_window=16):
        df = dataset.load_dataframe(filename)
        rows = len(df)
        tup_ranges = []
        for _ in range(num_samples):
            a = random.randint(0,rows-min_window)
            b = a + random.randint(min_window, rows-a)
            tup_ranges.append((a,b))
        samples = []
        for window in tup_ranges:
            a,b = window
            dfs = dataset._compress(df[a:b])
            if dfs.delta_t < max_window:
                samples.append(dfs)
        sample_df = pd.DataFrame(samples)
        return sample_df
        
    @staticmethod
    def _compress(df):
        # gather data to re-insert
        temp0 = df.temp.iloc[0]
        d_temp = df.temp.iloc[-1] - temp0
        d_time = df.delta_t.iloc[:-1].sum()

        # compress
        dfs = df[:-1].sum(axis=0)
        dfs = dfs/d_time

        # re-insert
        dfs.temp = temp0
        dfs.delta_t = d_time/d_time
        dfs = dfs.append(pd.Series(d_temp/d_time, index=['d_temp']))
        return dfs

    def trace_test(self, predictor):
        file = random.choice(self.test_files)
        #df = dataset._delta_temp_column(file)
        df = dataset.load_dataframe(file)
        delta_time = np.insert(df.delta_t.to_numpy()[:-1],0,0)
        time_axis = np.cumsum(delta_time)
        truth_temp = df.temp.to_numpy()
        '''
        pred_dt = predictor(df)
        t0 = df.temp.iloc[0]
        pred_temp = np.cumsum(pred_dt[:-1]) + t0
        pred_temp = np.insert(pred_temp,0,t0)
        '''
        pred_dt = [df.temp.iloc[0]]
        for i in range(len(df)):
            curdf = df[i:i+1]
            d_time = curdf.iloc[0].delta_t
            dempdf = curdf.copy()
            curdf = curdf/d_time
            curdf.update({'temp':[pred_dt[-1]]})
            this_pred_temp = predictor(curdf)[0]
            new_temp = pred_dt[-1] + this_pred_temp
            pred_dt.append(new_temp)
        pred_temp = np.array(pred_dt)

        return (df, truth_temp, pred_temp, time_axis)

    def trace_manual(self, df, predictor):
        delta_time = np.insert(df.delta_t.to_numpy()[:-1],0,0)
        time_axis = np.cumsum(delta_time)
        truth_temp = df.temp.to_numpy()
        
        pred_dt = [df.temp.iloc[0]]
        for i in range(len(df)):
            curdf = df[i:i+1]
            d_time = curdf.iloc[0].delta_t
            curdf = curdf/d_time
            curdf.update({'temp':[pred_dt[-1]]})
            this_pred_temp = predictor(curdf)[0]
            new_temp = pred_dt[-1] + this_pred_temp
            pred_dt.append(new_temp)
        pred_temp = np.array(pred_dt)

        return (truth_temp, pred_temp, time_axis)


import matplotlib.pyplot as plt
if __name__ == '__main__':
    ds = dataset()
    df = dataset.load_dataframe(random.choice(ds.train_files))
    df = df.head(3)
    dfc = dataset._compress(df)
    print(dfc)


    


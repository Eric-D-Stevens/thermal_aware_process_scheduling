from DS import DataStream
import numpy as np
import matplotlib.pyplot as plt
from sklearn import linear_model
from sklearn.preprocessing import MinMaxScaler, PolynomialFeatures
from sklearn.model_selection import train_test_split
import pandas as pd
from sklearn.svm import SVR

size = 500 
c = DataStream('c4t120.csv').rand_df(size)
d = DataStream('d4t120.csv').rand_df(size)
i = DataStream('i4t120.csv').rand_df(size)
m = DataStream('m4t120.csv').rand_df(size)

data = pd.concat([c,d,i,m])
#print(data.columns)


y = data.del_temp
X = data.drop('del_temp', axis=1)

cy = c.del_temp
cX = c.drop('del_temp', axis=1)


#print(data.shape)

scaler = MinMaxScaler()
X = scaler.fit_transform(X)
#print(data)
#print(scaled_data)
#print(scaler.get_params())


X_train, X_test, y_train, y_test = \
    train_test_split(X, y, test_size=0.2)


LM = linear_model.LinearRegression()
LM = linear_model.Ridge(alpha=10)
LM.fit(X_train, y_train)

#[print(i) for i in z]
#print(LM.coef_)

'''
y_pred = LM.predict(X_test)
plt.scatter(X_test.temp0, y_test,  color='black')
plt.plot(X_test.temp0, y_pred, color='blue', linewidth=3)
plt.show()

cy_hat = LM.predict(scaler.transform(cX))
plt.scatter(range(len(cy_hat)), (cy_hat-cy)/cy)
plt.ylim(ymin=0)
plt.show()
'''

def get_plot(filename):
    tc = DataStream(filename)
    t0 = tc.df.iloc[0].tempature/1000
    print(t0)
    cd = tc.create_test_data()
    cnorm = scaler.transform(cd)
    cy = LM.predict(cnorm)
    cscy = np.cumsum(cy)+t0
    plt.plot(cscy,'r')
    plt.plot(tc.df.tempature/1000,'g')
    plt.ylim(ymin=50, ymax=80)
    plt.title(filename)
    plt.show()




z = zip(data.columns, LM.coef_)
for i in z:
    print("%s:\t%f" % i)

get_plot('c4t120.csv')


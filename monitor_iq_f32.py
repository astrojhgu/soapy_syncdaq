#!/usr/bin/env python3

import matplotlib
matplotlib.use("tkcairo")
import numpy as np
import matplotlib.pyplot as plt
import time
import os
import sys
nch = 2048
file1 = sys.argv[1] if len(sys.argv)>=3 else 'a.bin'
file2 = sys.argv[2] if len(sys.argv)>=3 else 'b.bin'
tmin=int(sys.argv[3]) if len(sys.argv)>=4 else 0
tmax=int(sys.argv[4]) if len(sys.argv)>=5 else 500

freq=(np.arange(nch)-nch//2)/nch*320.0
#plt.ion()   # interactive mode
fig = plt.figure(figsize=(12, 6))


def safe_read(fname, dtype):
    """读取文件，异常时返回 None"""
    
    try:
        if not os.path.exists(fname):
            return None
        data = np.fromfile(fname, dtype=dtype)
        return data
    except Exception:
        return None
    


print("Reading files...")

data1 = safe_read(file1, dtype='float32')
data2 = safe_read(file2, dtype='float32')
# 判断读文件是否成功

data1 = data1[::2]+1j*data1[1::2]
data2 = data2[::2]+1j*data2[1::2]



x1 = data1.reshape((-1, nch))
x2 = data2.reshape((-1, nch))
n1=1
n2=1
#n1=np.sum(np.sum(np.abs(x1), axis=1)!=0)
#n2=np.sum(np.sum(np.abs(x2), axis=1)!=0)

# 计算相关
fft1 = np.fft.fftshift(np.fft.fft(x1, axis=1), axes=1)
fft2 = np.fft.fftshift(np.fft.fft(x2, axis=1), axes=1)
#fft1 = np.fft.fft(x1, axis=1)
#fft2 = np.fft.fft(x2, axis=1)
corr = np.sum(fft1 * np.conj(fft2), axis=0)
spec1= np.sum(fft1 * np.conj(fft1), axis=0)/n1
spec2= np.sum(fft2 * np.conj(fft2), axis=0)/n2


# 绘图
print("Current time:", time.strftime("%Y-%m-%d %H:%M:%S"))
plt.clf()

plt.subplot(231)
plt.plot(np.degrees(np.angle(corr)))
plt.ylim(-10, 10)

plt.xlabel('Channel id')
plt.ylabel('phase (deg)')
plt.title("Phase")


# Phase difference plot
plt.subplot(232)
plt.plot(np.degrees(np.angle(data1)))
plt.plot(np.degrees(np.angle(data2)))
plt.ylim(-180, 180)
if tmin<0:
    plt.xlim(len(data1)+tmin, len(data1)+tmax)
else:
    plt.xlim(tmin, tmax)
    
plt.xlabel('Channel id')
plt.ylabel('phase (deg)')
plt.title("Phase")

# Phase difference plot
plt.subplot(233)

plt.plot(freq, np.log10(spec1.real)*10)
plt.plot(freq, np.log10(spec2.real)*10)
plt.xlabel('$f-f_{lo}$ (MHz)')
plt.ylabel('ampl (dB)')
plt.title("Amplitude")


# Time-domain samples
plt.subplot(234)
plt.plot(data1.real, label='r1')
plt.plot(data1.imag, label='i1')

plt.plot(data2.real, label='r2')
plt.plot(data2.imag, label='i2')



if tmin<0:
    plt.xlim(len(data1)+tmin, len(data1)+tmax)
else:
    plt.xlim(tmin, tmax)
plt.xlabel("time (pt)")
plt.ylabel("y")
#plt.xlim((tmin, tmax))
#plt.ylim((-15000,15000))
plt.title("Raw Samples")


plt.subplot(235)
plt.plot(data2.real-data1.real, label='diff I')
plt.plot(data2.imag-data1.imag, label='diff Q')

plt.legend()
plt.tight_layout()

plt.show()
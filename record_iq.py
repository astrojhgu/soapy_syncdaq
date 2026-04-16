#!/usr/bin/env python3

import argparse
import sys

import numpy  # use numpy for buffers
import SoapySDR
from SoapySDR import *  # SOAPY_SDR_ constants

parser = argparse.ArgumentParser()

parser.add_argument("--ctrl-ip", type=str, required=True)
parser.add_argument("--freq", type=float, required=True)
parser.add_argument("--shifts", default="10:9:14")
parser.add_argument("--firshift", default="15")
parser.add_argument("--port_id", default="0")
parser.add_argument("--outfile", default="sdr_data.bin")
parser.add_argument("--buffer-size", type=int, default=8192)
parser.add_argument("--nbuf-to-rec", type=int, default=100)
args = parser.parse_args()

shifts = args.shifts
firshift = args.firshift
outfile = args.outfile
buffer_size = args.buffer_size
nbuf_to_rec = args.nbuf_to_rec
port_id = args.port_id
ctrl_ip = args.ctrl_ip
freq = args.freq
#
#
# enumerate devices
results = SoapySDR.Device.enumerate()
for result in results:
    print(result)
sdr_args = dict(driver="syncdaq", ctrl_ip=ctrl_ip, shifts=shifts, firshift=firshift)
sdr = SoapySDR.Device(sdr_args)
print("------------------------------")
print(sdr)

print(sdr.listAntennas(SOAPY_SDR_RX, 0))
print(sdr.listGains(SOAPY_SDR_RX, 0))
freqs = sdr.getFrequencyRange(SOAPY_SDR_RX, 0)
for freqRange in freqs:
    print(freqRange)
# sdr.setSampleRate(SOAPY_SDR_RX, 0, 50e6)
print(sdr.getSampleRate(SOAPY_SDR_RX, 0))
sdr.setFrequency(SOAPY_SDR_RX, 0, freq)

# sys.exit()
buff = numpy.array([0] * buffer_size, numpy.complex64)
rxStream = sdr.setupStream(SOAPY_SDR_RX, SOAPY_SDR_CF32)
sdr.activateStream(rxStream)  # start streaming
# 创建一个二进制文件来保存接收到的数据

with open(outfile, "wb") as f:
    for i in range(nbuf_to_rec):
        sr = sdr.readStream(rxStream, [buff], len(buff))
        # 插入一个写入语句，要求保存为二进制文件
        if sr.ret > 0:
            # 3. 将 numpy 数组转换为原始字节流并写入文件
            f.write(buff[: sr.ret].tobytes())

print(sdr.getSampleRate(SOAPY_SDR_RX, 0))
sdr.deactivateStream(rxStream)

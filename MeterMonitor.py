# -*- coding: utf-8 -*-

# Energy Monitor
# by Franz Stoiber
# The commercial use of this code is not allowed

# Documentation
# sudo apt install python3-pycryptodome

# set DataPort
DataPort = 5000
# setEVNKey
EVNKey = '0EAF2F77101465606630CE80E1xxxxxx'

import datetime
import time
import sys
from subprocess import *
import socket
from binascii import *
from cryptography.hazmat.primitives.ciphers.aead import AESGCM
from Cryptodome.Cipher import AES

PrgName = 'EnergyMonitor'
PrgVersion = 'V1.0'
print('> ' + PrgName + ' ' + PrgVersion + ' started')

#read IP
Result = str(Popen(['ifconfig'], stdout=PIPE).communicate()[0].decode())
Pos1 = Result.find('.') - 3
Pos2 = Result.find(' ', Pos1)
IP = Result[Pos1:Pos2]
BufferSize  = 1024

# create a datagram socket and bind it
UDPServerSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
UDPServerSocket.bind((IP, DataPort))
print('udp data server listening on ' + IP + ':' + str(DataPort) + ' for m-bus data')

# define unit names
UnitName = {
	27: 'W',  # 0x1b
	30: 'Wh', # 0x1e
	33: 'A',  #0x21
	35: 'V',  #0x23
	255: ''   # 0xff: no unit, unitless
} # https://www.dlms.com/files/Blue_Book_Edition_13-Excerpt.pdf

def getVal(Reading):
	Val = None
	Length = None
	ValType = int(Reading[0:2], 16)
	if ValType == 0x06: #uint32
		Val = int(Reading[2:10], 16)
		Length = 10
	elif ValType == 0x12: #uint16
		Val = int(Reading[2:6], 16)
		Length = 6
	elif ValType == 0x10: #int16
		Val = Hex16ToInt(Reading[2:6])
		Length = 6
	elif ValType == 0x0f: #int8
		Val = Hex8ToInt(Reading[2:4])
		Length = 4
	elif ValType == 0x16: #int8
		Val = int(Reading[2:4], 16)
		Length = 4
	return(Val, Length)
	
def getReading(ID):
	Value = None
	Unit = None
	Start = Data.find(ID)
	Reading = Data[Start + 12:Start + 34]	
	PosVal = 0
	Response = getVal(Reading)
	Val = Response[0]	
	Length = Response[1]
	Reading = Reading[Length:]
	PosExp = Reading.find('0202')
	Reading = Reading[4:]
	Response = getVal(Reading)
	Exp = Response[0]
	Length = Response[1]
	Reading = Reading[Length:]
	PosUnit = Reading.find('16')
	Response = getVal(Reading)
	try:
		Unit = UnitName[Response[0]]
	except:
		pass
	if Val != None and Exp != None:
		Value = Val * 10**Exp	
	return(Value, Unit)
					
def Hex8ToInt(Value):
	Val = int(Value, 16)
	return -(Val & 0x80) | (Val & 0x7f)

def Hex16ToInt(Value):
	Val = int(Value, 16)
	return -(Val & 0x8000) | (Val & 0x7fff)
 
# initialize values
LastFrameNr = -1
EncryptKey = unhexlify(EVNKey)
        
# listen for incoming frames
while True:
	bytesAddressPair = UDPServerSocket.recvfrom(BufferSize)
	Frame = bytesAddressPair[0]	

	#check frame length
	if len(Frame) != 282:
		LOG.error('frame length invalid')
		continue
		
	#select fields from frame
	MBusStart = Frame[0:4]
	SystemTitle = Frame[11:19]
	FrameCounter = Frame[22:26]
	FrameNr= FrameCounter[0] << 24 | FrameCounter[1] << 16 | FrameCounter[1] << 8 | FrameCounter[3]
	EncryptedData = Frame[26:280]
	Checksum = Frame[280]
	MBusStop = Frame[281]
	
	#check some fields
	if MBusStart.hex() != '68010168':
		LOG.error('frame start bytes invalid')
		continue
	if MBusStop != 0x16:
		LOG.error('frame stop byte invalid')
		continue
	#calculate checksum
	#if Checksum	...
	if LastFrameNr >= 0 and FrameNr != LastFrameNr + 1:
		LOG.info(str(FrameNr - LastFrameNr + 1) + ' lost frames')
	LastFrameNr= FrameNr
	
	#encrypt data
	InitVector = SystemTitle + FrameCounter
	Cipher = AES.new(EncryptKey, AES.MODE_GCM, nonce = InitVector)
	Data = Cipher.decrypt(EncryptedData).hex()
		
	#read readings
	#DateTime
	Year = None
	Month = None
	Day = None
	Weekday = None
	Hour = None
	Minute = None
	Second = None
	Hundredths = None
	Offset = None
	Clockstatus = None
	Reading = Data[12:36]
	try:
		Year = int(Reading[0: 4], 16)
		Month = int(Reading[4:6], 16)
		Day = int(Reading[6:8], 16)
		Weekday = int(Reading[8:10], 16)
		Hour = int(Reading[10:12], 16)
		Minute = int(Reading[12:14], 16)
		Second = int(Reading[14:16], 16)
		Hundredths = int(Reading[16:18], 16)
		Offset = Hex16ToInt(Reading[18:22])
		if Reading[22] == '0':
			Clockstatus = ''
		else:
			Clockstatus = 'DST'
	except:
		print('error in date time')
		
	#Wirkenergy +
	WhPSum = getReading('0100010800ff')
	
	#Wirkenergy - 
	WhNSum = getReading('0100020800ff')
	
	#Momentanleistung +
	WP = getReading('0100010700ff')
	
	#Momentanleistung -
	WN = getReading('0100020700ff')
	
	#Spannung L1
	UL1 = getReading('0100200700ff')
	
	#Spannung L2
	UL2 = getReading('0100340700ff')
	
	#Spannung L3
	UL3 = getReading('0100480700ff')
	
	#Strom L1
	IL1 = getReading('01001f0700ff')
	
	#Strom L2
	IL2 = getReading('0100330700ff')
	
	#Strom L3
	IL3 = getReading('0100470700ff')
	
	#Leistungsfaktor
	PF = getReading('01000d0700ff')
	
	try:
		print()
		print(Year, Month, Day, Weekday, Hour, Minute, Second, Hundredths, Offset, Clockstatus)
		print('Wirkenergie+:', WhPSum[0], WhPSum[1])
		print('Wirkenergie-:', WhNSum[0], WhNSum[1])
		print('Momentanleistung+:', WP[0], WP[1])
		print('Momentanleistung-:', WN[0], WN[1])
		print('Spannung L1:', round(UL1[0], 1), UL1[1])
		print('Spannung L2:', round(UL2[0], 1), UL2[1])
		print('Spannung L3:', round(UL3[0], 1), UL3[1])
		print('Strom L1:', round(IL1[0], 3), IL1[1])
		print('Strom L2:', round(IL2[0], 3), IL2[1])
		print('Strom L3:', round(IL3[0], 3), IL3[1])
		print('Leistungsfaktor:', round(PF[0], 3), PF[1])
		print('Momentanleistung:', round(WP[0] - WN[0], 0), WP[1])
	except:
		print('error in reading')

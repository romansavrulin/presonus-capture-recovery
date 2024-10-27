1459A800  d=0x8000

145A2800

145AA800

145B2800

145BA800

Data end: 146AA800


Last correct byte for track1: 146B2800

Next correct byte for track1: 146B2800 


2F940А800
2F940A800
2F9402800
2F93FA800 

18000 - Инкремент между каналами

```
To align for removing distortions, you should step with `Data Block Size`
To align for removing skips, you should step with `Chunk size`
To move channels forward you should step back `Channel Block size`
To move in time you should step with `Data Block Size * 3` (0x1980000)

Chunk size: 0x8000
Channel Block size: 0x40000
Chunks per Channel Block: 0x8
Data Block Size: 0x880000
----- Wav Headers ---- (0x110000) (Chunk size * nChannels)
0x000000: Wav Header Ch 1
0x008000: Wav Header Ch 2
....
0x108000: Wav Header Ch 34
----- Data Block 1 --- (0x880000) (Channel Block size * nChannels)
  ------- Ch 1 ------- (0x40000)
0x110000: Ch 1 Chunk 1
0x118000: Ch 1 Chunk 2
....
0x148000: Ch 1 Chunk 8
  ------- Ch 2 ------- (0x40000)
0x150000: Ch 2 Chunk 1
0x158000: Ch 2 Chunk 2
....
0x148000: Ch 2 Chunk 8
....
0x988000: Ch 34 Chunk 8
  --------------------
----- Data Block 2 ---
0x990000: Ch 1 Chunk 1
```

нет искажений, но есть затыки
```
./build/Debug/bin/cpp-cmake-template -i ~/Downloads/Kvart-recovery/kvart.dd -c 100 -m 3 -d ~/Downloads/Kvart-recovery/Kvart\ Ben/Audio/ -s 20 -o 0x2F9402800 -t 34
```

+1 канал = offs-Channel Block size: 0x40000
```
./build/Debug/bin/cpp-cmake-template -i ~/Downloads/Kvart-recovery/kvart.dd -c 100 -m 3 -d ~/Downloads/Kvart-recovery/Kvart\ Ben/Audio/ -s 21 -o 0x2F93C2800 -t 34
```

mains at 21, 22, 23 distorted
``` 
./build/Debug/bin/cpp-cmake-template -i ~/Downloads/Kvart-recovery/kvart.dd -c 100 -m 4 -d ~/Downloads/Kvart-recovery/Kvart\ Ben/Audio/ -o 0x2F828A800 -t 34
```

all good, mains at 33, 34
``` 
./build/Debug/bin/cpp-cmake-template -i ~/Downloads/Kvart-recovery/kvart.dd -c 100 -m 4 -d ~/Downloads/Kvart-recovery/Kvart\ Ben/Audio/ -o 0x2F7FCA800 -t 34
```
# Simple-NOAA-APT-encoder-for-HackRF

this is a NOAA apt signal generator for HackRF and makes wav file(44100Hz 8bit) and I/Q file(bin for HackRF).

usege

>noaa_bin input.bmp output.wav output.bin


>hackrf_transfer -t test.bin -f 137100000 -s 2822400 -a 1 -x 47 -R

NOAA15->137620000
NOAA18->137912000
NOAA19->137100000

#this is a prototype.

caution! this need to be compiled under 32bit environment.
if you get compile in 64 bit, change all 

  unsigned long --> unsinged int


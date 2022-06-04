The log file from MHI-AC-Trace.ino has the following format:

One row has 43 bytes in hexadecimal format separated by a space and represents one or more identical MOSI/MISO frames. 
If it represents more than one frame, then the repetition number tells you how often it appeared.

byte # | meaning
-------| --------
0 | highByte(packet_cnt)
1 | lowByte(packet_cnt)
2-21 | MOSI bytes 0-19
22-41 | MISO bytes 0-19
42 | repetitionNo`

packet_cnt starts with a random value


packet_cnt and repetitionNo are usually redundant, but it can be used in [MHI-AC-Trace.ino](https://github.com/absalom-muc/MHI-AC-Trace/blob/main/src/MHI-AC-Trace.ino) for detection of lost frames.

Please check the example [MHI-ErrOpData.log](https://github.com/absalom-muc/MHI-AC-Trace/blob/main/src/MHI-ErrOpData.log).
It represents a read of the ErrOpData for my AC.




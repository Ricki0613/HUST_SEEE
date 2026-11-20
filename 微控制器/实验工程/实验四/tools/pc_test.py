"""Usage: python pc_test.py COM5 115200 4000 (requires pyserial)."""
import sys,time,serial
port,rate,length=sys.argv[1],int(sys.argv[2]),int(sys.argv[3])
if not 1<=length<=4096: raise ValueError("length must be 1..4096")
body=bytes(65+i%26 for i in range(length)); expected=b'ok+'+body+b'\r\n'
with serial.Serial(port,rate,timeout=15,dsrdtr=False,rtscts=False) as s:
    time.sleep(2);s.reset_input_buffer()
    start=time.perf_counter();s.write(body+b'\r\n');s.flush()
    reply=s.read(len(expected));elapsed=time.perf_counter()-start
    print("baud",rate,"payload",length,"received",len(reply),"PC round trip (s)",round(elapsed,4))
    print("PASS" if reply==expected else "FAIL: "+repr(reply[:60]))
    sys.exit(0 if reply==expected else 1)

# DMA driver test

There are two drivers example in this chapter. After running `make` command,
there will be two modules:

* imx-sdma-scatter-gather.ko
* imx-sdma-single.ko

The fist module implements the scatter/gather DMA, and the second one implement
a single buffer mapping. Both rely on i.MX6 from NXP.
These two modules are mutally excluse. These can't be load at the same time.
Prior to load one module, make sure the other one is not loaded.

Whatever module is loaded, it will create a character device, `/dev/sdma_test`.

```bash
# udevadm info /dev/dma_test 
P: /devices/virtual/dma_test/sdma_test
N: sdma_test
E: DEVNAME=/dev/dma_test
E: DEVPATH=/devices/virtual/dma_test/dma_test
E: MAJOR=244
E: MINOR=0
E: SUBSYSTEM=sdma_test
```

For testing purpose, one just has to write a dummy string into it, and then
read anything from it.

## Single mapping DMA

Below is what one can do for testing the single buffer mapping module:

```bash
# insmod dma-single-buffer.ko 
[  315.517196] DMA-TEST: DMA test major number = 234
[  315.524313] DMA-TEST: DMA test Driver Module loaded
# echo "" > /dev/dma_test  
SDMA test major number = 244
SDMA test Driver Module loaded
[  317.928507] DMA-TEST: Initializing buffer
[  317.932618] DMA-TEST: Dumping WBUF initialized buffer
[  317.937780] DMA-TEST: [0000] 56 56 56 56 56 56 56 56
[  317.942850] DMA-TEST: [0008] 56 56 56 56 56 56 56 56
[...]
[  323.106945] DMA-TEST: [8176] 56 56 56 56 56 56 56 56
[  323.111999] DMA-TEST: [8184] 56 56 56 56 56 56 56 56
[  323.117055] DMA-TEST: Buffer initialized
[  323.121141] DMA-TEST: Got DMA channel 3
[  323.125058] DMA-TEST: DMA channel configured
[  323.129412] DMA-TEST: DMA mappings created
[  323.133634] DMA-TEST: Got this cookie: 2
[  323.137634] DMA-TEST: waiting for DMA transaction...
[  323.137781] DMA-TEST: in dma_m2m_callback
[  323.146783] DMA-TEST: Checking if DMA succeed ...
[  323.151600] DMA-TEST: buffer copy passed!
[  323.155706] DMA-TEST: Dumping RBUF DMA buffer
[  323.160148] DMA-TEST: [0000] 56 56 56 56 56 56 56 56
[  323.165207] DMA-TEST: [0008] 56 56 56 56 56 56 56 56
[  323.170262] DMA-TEST: [0016] 56 56 56 56 56 56 56 56
[...]
buffer copy passed!

# rmmod dma-single-buffer.ko
```

# c2clat

A tool to measure CPU core to core latency (inter-core latency).

## Build

### console
```console
g++ -O3 -DNDEBUG -march=native -flto -flto-partition=none c2clat.cpp -o c2clat -pthread
```

### cmake + ninja
```console
./tools/ninja-cmake-builder.sh -DCMAKE_BUILD_TYPE=Release
```

### Crossplatform [Dockcross](https://github.com/dockcross/dockcross)
```console
./tools/dockcross-builder.sh linux-arm64:latest -DCMAKE_BUILD_TYPE=Release
```

Example usage:

```console
$ ./c2clat 
 CPU    0    1    2    3    4    5    6    7    8    9   10   11   12   13   14   15
   0    0   10   26   24   24   26   25   26   24   24   25   25   23   23   23   24
   1   10    0   26   26   26   26   24   25   23   23   25   24   24   23   22   24
   2   26   26    0   10   25   25   26   25   24   25   25   24   24   24   24   25
   3   24   26   10    0   26   26   25   25   25   25   24   25   23   23   24   24
   4   24   26   25   26    0   10   26   24   25   18   24   24   23   24   24   24
   5   26   26   25   26   10    0   24   24   25   25   22   24   24   24   24   25
   6   25   24   26   25   26   24    0   10   23   24   22   23   22   23   22   21
   7   26   25   25   25   24   24   10    0   22   23   24   24   23   23   23   22
   8   24   23   24   25   25   25   23   22    0   10   23   23   22   22   20   21
   9   24   23   25   25   18   25   24   23   10    0   23   23   22   23   22   23
  10   25   25   25   24   24   22   22   24   23   23    0   10   21   21   21   21
  11   25   24   24   25   24   24   23   24   23   23   10    0   21   22   22   23
  12   23   24   24   23   23   24   22   23   22   22   21   21    0   10   21   21
  13   23   23   24   23   24   24   23   23   22   23   21   22   10    0   21   22
  14   23   22   24   24   24   24   22   23   20   22   21   22   21   21    0   10
  15   24   24   25   24   24   25   21   22   21   23   21   23   21   22   10    0
```

Create plot using [gnuplot](http://gnuplot.sourceforge.net/):

```console
c2clat -p | gnuplot -p
```

![Plot of inter-core latency](https://github.com/bensuperpc/c2clat/blob/main/c2clat.png)

If you want to run on a subset of cores use [taskset](https://www.man7.org/linux/man-pages/man1/taskset.1.html):

```console
$ taskset -c 10-11 ./c2clat
 CPU   10   11
  10    0   52
  11   52    0
```

If you want to label the heatmap plot replace the plot command with:

```gnuplot
plot '$data' matrix rowheaders columnheaders using 2:1:3 with image, \
     '$data' matrix rowheaders columnheaders using 2:1:(sprintf("%g",$3)) with labels
```

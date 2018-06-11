
Usage
-----
./ncs-googlenet-check [-l<loglevel>] [[-n<count>] -g<graph file> -c<categories file> -i<image>]
                <loglevel> API log level
                <count> is the number of inference iterations, default 2
                <graph file> path to GoogleNet graph file
                <categories file> path to categories file
                if graph is not provided, app will just open and close the NCS device
Example
-------
ncs-googlenet-check.exe -n 1 -g ../../caffe/GoogLeNet/graph -i ../../data/images/512_Ball.jpg  -c ../../data/ilsvrc12/synset_words.txt

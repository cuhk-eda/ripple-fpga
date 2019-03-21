# RippleFP

RippleFPGA is for generating VLSI placement of UltraScale FPGAs.
More details are in the following papers:
* Chak-Wa Pui, Gengjie Chen, Wing-Kai Chow, Jian Kuang, Ka-Chun Lam, Peishan Tu, Hang Zhang, Evangeline F.Y. Young, Bei Yu, [RippleFPGA: A Routability-Driven Placement for Large-Scale Heterogeneous FPGAs](http://ieeexplore.ieee.org/document/7827644/), 
IEEE/ACM International Conference on Computer-Aided Design, pp. 67:1-67:8, Nov. 7-10, 2016.
* Chak-Wa Pui, Gengjie Chen, Yuzhe Ma, Evangeline F.Y. Young, Bei Yu, [Clock-Aware UltraScale FPGA Placement with Machine Learning Routability Prediction](http://ieeexplore.ieee.org/document/8203880/), 
IEEE/ACM International Conference on Computer-Aided Design, pp. 929-936, Nov. 13-16, 2017.
* Gengjie Chen, Chak-Wa Pui, Wing-Kai Chow, Ka-Chun Lam, Jian Kuang, Evangeline F. Y. Young and Bei Yu, [RippleFPGA: Routability-Driven Simultaneous Packing and Placement for Modern FPGAs](http://ieeexplore.ieee.org/document/8122004/), 
IEEE Transactions on Computer-Aided Design of Integrated Circuits and Systems, vol. 37, no. 10, pp. 2022–2035, 2018.

## Quick Start

The simplest way to build and run RippleFPGA is as follows.
~~~
$ git clone https://github.com/jordanpui/ripplefpga
$ cd ripplefpga
$ make mode=release_mt
$ cd bin
$ ./placer -aux toy_example/design.aux -out toy_example.out
~~~

## Building RippleFPGA

**Step 1:** Download the source codes. For example,
~~~
$ git clone https://github.com/jordanpui/ripplefpga
~~~

**Step 2:** Go to the project root and build by
~~~
$ cd ripplefpga
$ make mode=release_mt
~~~

Note that this will generate a folder `bin` under the root, which contains binaries and auxiliary files.
More details are in `Makefile`.

### Dependencies

* g++ (version 4.8.0) or other working c++ compliers
* Boost (version >= 1.58)

## Runing RippleFPGA

### Toy

Go to the `bin` directory and run binary `placer` with a toy design:
~~~
$ cd bin
$ ./placer -aux toy_example/design.aux -out toy_example.out
~~~

### Batch Test

Our placer is based on bookshelf format and was tested on two contest benchmarks [ISPD'16 Contest](http://www.ispd.cc/contests/16/) and [ISPD'17 Contest](http://www.ispd.cc/contests/17/), which can be downloaded via Dropbox ([ISPD16](https://www.dropbox.com/sh/9c74a6f4o0rrd2t/AAA3V_fiP15pV20fV62apLoqa?dl=0), [ISPD17](https://www.dropbox.com/sh/9aranna360wnez2/AABYc5n1Sak3AY3m25eJ7Nyka?dl=0)).
After download the benchmarks and place them under folder `BM_DIR`, you can use our script `run.sh`:
~~~
$ cd bin
$ export BENCHMARK_PATH=BM_DIR
$ ./run all
~~~

## Modules

* `src`: c++ source codes
    * `alg`: external algorithm packages
    * `cong`: implemetation of congestion estimation
    * `db`: implementation of database
    * `dp`: implementation of detailed placement
    * `gp`: implementation of global placement
    * `lg`: implementation of legalization
    * `lgclk`: implementation of legailzation related to clock constraints
    * `pack`: implementation of packing
    * `utils`: implementation of utilizations
* `toy_example`: toy example in bookshelf format

## License
READ THIS LICENSE AGREEMENT CAREFULLY BEFORE USING THIS PRODUCT. BY USING THIS PRODUCT YOU INDICATE YOUR ACCEPTANCE OF THE TERMS OF THE FOLLOWING AGREEMENT. THESE TERMS APPLY TO YOU AND ANY SUBSEQUENT LICENSEE OF THIS PRODUCT.



License Agreement for RippleFPGA



Copyright (c) 2019 by The Chinese University of Hong Kong



All rights reserved



ABSD LICENSE (adapted from the original BSD license) Redistribution of the any code, with or without modification, are permitted provided that the conditions below are met. 



Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.



Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.



Use is limited to non-commercial products only. Users who are interested in using it in commercial products must notify the author and request separate license agreement.



Neither the name nor trademark of the copyright holder or the author may be used to endorse or promote products derived from this software without specific prior written permission.



Users are entirely responsible, to the exclusion of the author, for compliance with (a) regulations set by owners or administrators of employed equipment, (b) licensing terms of any other software, and (c) local, national, and international regulations regarding use, including those regarding import, export, and use of encryption software.



THIS FREE SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR ANY CONTRIBUTOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, EFFECTS OF UNAUTHORIZED OR MALICIOUS NETWORK ACCESS; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
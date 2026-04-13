all: libsoapy_syncdaq.so

LIBS=-L ./lib -lSoapySDR -lyaml-cpp -L ../syncdaq/target/release -lsyncdaq
CFLAGS=-g -I ../syncdaq/include -I ../sdaa_ctrl/include

#ddc_kernel.o: ddc_kernel.cu
#	nvcc -c $< -o $@ $(OPT) --cudart=static --cudadevrt=none $(CFLAGS)

syncdaq.o: syncdaq.cpp
	g++ -fPIC -c --std=c++23 $< -o $@ -O3 $(CFLAGS)

utils.o: utils.cpp utils.hpp
	g++ -fPIC -c --std=c++23 $< -o $@ -O3 $(CFLAGS)

libsoapy_syncdaq.so: syncdaq.o utils.o
	g++ -fPIC --shared  $^ -o $@ $(LIBS)

clean:
	rm -f *.o

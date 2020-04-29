/home/amd/rocm/aomp/bin/clang  -O2  -target x86_64-pc-linux-gnu -fopenmp -fopenmp-targets=amdgcn-amd-amdhsa -Xopenmp-target=amdgcn-amd-amdhsa -march=gfx906   helloworld.c -o helloworld   -save-temps

llvm-objcopy -O binary --only-section=.note  a.out-openmp-amdgcn-amd-amdhsa-gfx906 helloworld.msgpack

cat <<EOF > helloworld_msgpack.h
extern unsigned char helloworld_msgpack[];
extern unsigned int helloworld_msgpack_len;
EOF

xxd --include helloworld.msgpack &> helloworld_msgpack.c

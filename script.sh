ulimit -c unlimited
ulimit -s unlimited
ulimit -d unlimited
ulimit -f unlimited



export OBJECT_MODE=64
export CC="/opt/IBM/openxlC/17.1.4/bin/ibm-clang_r "
export CXX="/opt/IBM/openxlC/17.1.4/bin/ibm-clang++_r "
export CFLAGS="-pthread -m64 -D_ALL_SOURCE -mcmodel=large -DFLATBUFFERS_LITTLEENDIAN=0 -Wno-deprecate-lax-vec-conv-all  -Wno-unused-but-set-variable -Wno-unused-command-line-argument -maltivec -mvsx  -Wno-unused-variable -Wno-unused-parameter -Wno-sign-compare -Wno-shorten-64-to-32"
export CXXFLAGS="-pthread -m64 -D_ALL_SOURCE -mcmodel=large -DFLATBUFFERS_LITTLEENDIAN=0 -Wno-deprecate-lax-vec-conv-all -Wno-unused-but-set-variable -Wno-unused-command-line-argument -maltivec -mvsx  -Wno-unused-variable -Wno-unused-parameter -Wno-sign-compare -Wno-shorten-64-to-32"
export LDFLAGS="-L$PWD/build/Linux/Release/ -L/opt/freeware/lib/pthread -L/opt/freeware/lib64 -L/opt/freeware/lib -L/Kushal/malloc/mimalloc/out/release -lpthread -lpython3.12"
export LIBPATH="$PWD/build/Linux/Release/"

export PATH="/opt/freeware/bin/:$PATH"


./build.sh \
	--config Release\
        --parallel 10 \
        --allow_running_as_root \
        --build_shared_lib \
        --skip_submodule_sync \
        --enable_pybind \
        --skip_tests \
        --cmake_extra_defines PYTHON_EXECUTABLE=/opt/freeware/bin/python3.12\
        --cmake_extra_defines CMAKE_AIX_SHARED_LIBRARY_ARCHIVE=ON \
        --cmake_extra_defines CMAKE_INSTALL_PREFIX=$PWD/install_dir || true

cd ./build/Linux/Release/

./onnxruntime_test_all --gtest_filter=FlatbufferUtilsTest.ExternalWriteReadWithLoadInitializers
./onnxruntime_test_all --gtest_filter=InternalTestingEP.TestSaveAndLoadOrtModel
./onnxruntime_test_all --gtest_filter=OrtModelOnlyTests.SparseInitializerHandling
./onnxruntime_test_all --gtest_filter=OrtModelOnlyTests.ValidateOrtFormatModelDoesNotRunOptimizersInFullBuild > ort_model_test_output.txt 2>&1
./onnxruntime_shared_lib_test --gtest_filter=CApiTest.TestLoadModelFromArrayWithExternalInitializersFromFileArray
./onnxruntime_shared_lib_test --gtest_filter=CApiTest.TestLoadModelFromArrayWithExternalInitializersFromFileArrayPathRobust
./onnxruntime_shared_lib_test --gtest_filter=CApiTest.TestLoadModelFromArrayWithExternalInitializersFromFileMmap
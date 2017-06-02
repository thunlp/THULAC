dst_dir=.
include_dir=include
src_dir=src
bin_dir=.
test_dir=test
thulac=g++ -std=c++11 -O3 -march=native -I $(include_dir)

# all: $(bin_dir)/thulac_test $(bin_dir)/train_c $(bin_dir)/thulac
all: $(bin_dir)/thulac $(bin_dir)/train_c $(bin_dir)/thulac_test $(bin_dir)/libthulac.so $(bin_dir)/test_case

$(bin_dir)/thulac: $(src_dir)/thulac.cc $(include_dir)/*.h
	$(thulac) $(src_dir)/thulac.cc -o $(bin_dir)/thulac

$(bin_dir)/train_c: $(src_dir)/train_c.cc $(include_dir)/*.h
	$(thulac) -o $(bin_dir)/train_c $(src_dir)/train_c.cc

$(bin_dir)/test_case: $(test_dir)/test_case.cpp $(include_dir)/*.h
	$(thulac) -o $(bin_dir)/test_case $(test_dir)/test_case.cpp

$(bin_dir)/thulac_test: $(src_dir)/thulac_test.cc $(include_dir)/*.h
	$(thulac) -o $(bin_dir)/thulac_test $(src_dir)/thulac_test.cc	

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S), Linux)
$(bin_dir)/libthulac.so: $(src_dir)/thulac_so.cc $(include_dir)/*.h
	$(thulac) $(include_dir)/thulac_lib.cc -shared -fPIC -Wl,-soname=libthulac.so.1 -o $(bin_dir)/libthulac.so
endif
ifeq ($(UNAME_S), Darwin)
$(bin_dir)/libthulac.so: $(src_dir)/thulac_so.cc $(include_dir)/*.h
	$(thulac) $(include_dir)/thulac_lib.cc -shared -fPIC -Wl,-install_name,libthulac.so.1 -o $(bin_dir)/libthulac.so
endif

clean:
	rm -f $(bin_dir)/thulac 
	rm -f $(bin_dir)/train_c 
	rm -f $(bin_dir)/thulac_test
	rm -f $(bin_dir)/libthulac.so


pack:
	tar -czvf THULAC_lite_c++_v1.tar.gz src Makefile doc README.md

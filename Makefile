dst_dir=.
src_dir=src
bin_dir=.
thulac=g++ -O3 -std=c++0x -march=native -I $(src_dir)

#all: $(dst_dir)/dat_builder $(dst_dir)/thulac $(dst_dir)/thulac_time $(dst_dir)/predict_c
all: $(bin_dir)/thulac $(bin_dir)/train_c

$(bin_dir)/thulac: $(src_dir)/thulac.cc $(src_dir)/*.h
	$(thulac) $(src_dir)/thulac.cc -o $(bin_dir)/thulac

$(bin_dir)/train_c: $(src_dir)/train_c.cc $(src_dir)/*.h
	$(thulac) -o $(bin_dir)/train_c $(src_dir)/train_c.cc

clean:
	rm $(bin_dir)/thulac 
	rm $(bin_dir)/train_c 

pack:
	tar -czvf THULAC_lite_c++_v1.tar.gz src Makefile doc README.md

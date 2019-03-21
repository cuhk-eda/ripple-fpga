SHELL=/bin/bash

OPT     := -O3 -DNDEBUG
LIBS	:= -pthread
ifeq ($(mode),debug)
    OPT     := -O0 -g -DWRITE -DWRITE_GROUP -DDRAW
    LIBS    += -lgd
else
ifeq ($(mode),release)
    OPT     += -static -DSINGLE_THREAD
else
ifeq ($(mode),release_mt)
	OPT     += -static
	LIBS	= -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
else
    OPT     += -DWRITE_GROUP
endif
endif
endif

target = ../bin

WFLAG 	= -Wall
DEPFLAG = -MMD -MP

CC		= g++-4.8 -std=c++11 $(OPT) $(WFLAG) $(DEPFLAG) $(INCLUDE)

INCLUDE = -I. -I../../boost_1_62_0
LIBS 	+= $(addprefix alg/, patoh/libpatoh.a lpsolve/lib64/liblpsolve55.a) \
			-lboost_system -ldl

CC_OBJS = main
UT_OBJS = $(addprefix utils/, log draw)
DB_OBJS = $(addprefix db/, db db_draw db_bookshelf site instance net group swbox clkrgn)
PC_OBJS = $(addprefix pack/, pack ble clb1 clb2 clb2_lutpg clb2_res_base clb2_res1 clb2_res2 pack_ble)
GP_OBJS = $(addprefix gp/, gp gp_data gp_main gp_qsolve gp_spread gp_region gp_setting)
LG_OBJS = $(addprefix lg/, lg_dp_utils lg lg_main lg_data lg_dp_chain)
CLK_OBJS = $(addprefix lgclk/, lgclk lgclk_data lgclk_colrgn lgclk_plan)
CONG_OBJS = $(addprefix cong/, cong_est_base cong_est_bb cong_gp cong_partition)
DP_OBJS = $(addprefix dp/, dp dp_pack dp_ble dp_data)
ALG_OBJS = $(addprefix alg/, matching bipartite)

OBJS = 	$(addsuffix .o, $(CC_OBJS) $(UT_OBJS) $(DB_OBJS) $(IO_OBJS) $(ALG_OBJS) \
		$(DP_OBJS) $(GP_OBJS) $(VI_OBJS) $(MM_OBJS) $(PC_OBJS) $(LG_OBJS) $(GR_OBJS) \
	   	$(CONG_OBJS) $(CLK_OBJS)) 

BFILE = placer

TAR_FILES = $(BFILE) run.sh eval.sh ../toy_example

.PHONY: all clean tags
all: $(BFILE)
	@date +'%D %T'
	mkdir -p $(target)
	cp -ur $(TAR_FILES) $(target)/

$(BFILE): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)

%.o : %.cpp
	$(CC) -o $@ -c $<

-include $(OBJS:.o=.d)

clean:
	rm -f {*,*/*,*/*/*}.{o,d} $(BFILE)

tags:
	cscope -Rbq
	ctags -R *.cpp *.h
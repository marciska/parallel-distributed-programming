
include stuff/psweep.mk

numactl:=numactl -iall --

# ---------------------------------------

out_dir:=output

# ----- parameter and command definitions -----

# from 2^8 to 2^23 elements
#p:=$(shell seq 7 16)
#powers:=$(foreach i,$(p),$(shell echo $$((1 << $(i)))))
# multiply each of 2^8 - 2^23 by 7/9 - 15/9
#n:=$(foreach p,$(powers),$(foreach o,$(shell seq -2 4),$(shell echo $$(($(p) * (9 + $(o)) / 9)))))
# from 2^8 to 2^16 elements, taking 6 points between 2^i and 2^(i+1)
a:=7
b:=17
s:=8
n:=$(shell python3 -c "for i in range($(a)*$(s),$(b)*$(s)): print(int(2.0**(i/$(s))))" | uniq)

parameters:=host try method n n_chains n_threads shuffle payload cpu_node mem_node prefetch

host:=$(shell hostname | tr -d [0-9])
ifeq ($(host),big)
events := l1d.replacement,l2_lines_in.all
endif
ifeq ($(host),p)
events := l1d.replacement,l2_lines_in.all
endif
ifeq ($(host),v)
events := l1d.replacement,l2_lines_in.all
endif
ifeq ($(host),login)
events := l1d.replacement,l2_lines_in.all
endif


### commands and outputs ###
cmd=(OMP_NUM_THREADS=$(n_threads) numactl -N $(cpu_node) -i $(mem_node) -- ./mem -m $(method) -n $(n) -c $(n_chains) -x $(shuffle) -l $(payload) -p $(prefetch) -r 5 -e $(events)) > $(output)
input=$(out_dir)/created
output=$(out_dir)/out_$(host)_$(method)_$(n)_$(n_chains)_$(n_threads)_$(shuffle)_$(payload)_$(cpu_node)_$(mem_node)_$(prefetch)_$(try).txt

## common parameters ##
#host:=$(shell hostname | tr -d [0-9])
cpu_node:=0
payload:=0
#payload:=1
try:=$(shell seq 1 5)

## effect of number of chains ##
method:=p
n_chains:=1 2 4 8 10 12 14
n_threads:=1
shuffle:=1
prefetch:=0
#mem_node:=0 1
mem_node:=0
#$(define_rules)

## effect of working set size ##
method:=p
n_chains:=1
n_threads:=1
shuffle:=1
prefetch:=0
#mem_node:=0 1
mem_node:=0
$(define_rules)

## effect of access methods ##
method:=p s r
n_chains:=1 2 4
n_threads:=1
shuffle:=1
prefetch:=0
mem_node:=0
#$(define_rules)

## effect of prefetch ##
method:=p
n_chains:=1 2 4
n_threads:=1
shuffle:=1
prefetch:=0 10
mem_node:=0
#$(define_rules)

## effect of sorted addresses ##
method:=p
n_chains:=1 2 4
n_threads:=1
shuffle:=0 1
prefetch:=0
mem_node:=0
#$(define_rules)

## many threads with pointers ##
method:=p
n_chains:=1 5 10
n_threads:=1 2 4 6 8 12 16
shuffle:=1
prefetch:=0
mem_node:=0
#$(define_rules)

## many threads with pointers ##
method:=s r
n_chains:=1
n_threads:=1 2 4 6 8 12 16
shuffle:=1
prefetch:=0
mem_node:=0
#$(define_rules)

$(out_dir)/created :
	mkdir -p $@

.DELETE_ON_ERROR:



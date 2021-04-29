TARGET=sight
OUTDIR=out
DB?=lldb
CT?=clang-tidy
SF!=find -E src -regex ".*\.((c|cc|cpp|cxx)|(h|hh|hpp|hxx))$$"
TF=$(OUTDIR)/$(TARGET)-trace.out

build:	config
	ninja -C $(OUTDIR)

compdb:	config
	ninja -C $(OUTDIR) -t compdb cc cxx > $(OUTDIR)/compile_commands.json
	ln -s -f $(OUTDIR)/compile_commands.json compile_commands.json

config:
	gn --args="enable_pkgconf=true debug_build=true" gen $(OUTDIR)

clean:
	rm -rfv $(OUTDIR)
	rm -fv compile_commands.json
	rm -rf .ccls-cache

check:	compdb
	$(CT) -checks=-*,clang-analyzer-*,cppcoreguidelines-*,modernize-*,performance-*,portability-*,readability-* $(SF)

# test:	build
# 	$(OUTDIR)/$(TARGET)-test

# bench:	build
# 	$(OUTDIR)/$(TARGET)-bench

run:	build
	$(OUTDIR)/$(TARGET) --logtostderr=true

debug:	build
	$(DB) $(OUTDIR)/$(TARGET)

size:	build
	bloaty $(OUTDIR)/$(TARGET)

strip:	build
	strip $(OUTDIR)/$(TARGET)

trace:	build
	ktrace -f $(TF) $(OUTDIR)/$(TARGET)
	kdump -f $(TF) | less

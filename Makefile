all: submodule

submodule:
	git submodule update --init
	cd mculib3/ && git checkout dvk
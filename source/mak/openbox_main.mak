


export TOOL_CHAIN := 4.9.1
export ENABLE_CA9_NEON := true

#export TOOL_CHAIN := 4.8.2
#export ENABLE_CA9_NEON := true

all:
	make -f openbox_tar.mak
	make -f openbox_build.mak
	
clean:
	make clean -f openbox_tar.mak


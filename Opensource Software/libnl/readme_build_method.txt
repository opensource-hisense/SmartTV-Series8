1,配置环境参数(file:libnl-3.2.25/run)
  修改TOOL_CHAIN_BIN_PATH/CROSS_COMPILE及其它需要定制的配置项
2,编译命令
  #source run
  #make
  #make install
3,修改库名称
  #cd lib
  #ln -s libnl-3.so libnl.so
  #ln -s libnl-genl-3.so libnl-genl.so
4,copy libnl.so/linl-genl.so -> oss/library/libnl/..
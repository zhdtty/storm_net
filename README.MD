#项目说明

	一个c++的rpc架子，方便server间交互。rpc的好处就是隐藏了网络层细节，自定义协议等一些繁琐而机械化的工作。框架通过接口定义文件定义接口，自动生成代码，隐藏了网络细节和协议。
	支持同步调用，异步调用和单向调用。

#安装、测试

	依赖： gcc4.8 cmake 2.8 protobuf flex bison

	*1.编译安装
		mkdir build
		cd build 
		cmake ..
		make -j4
		sudo make install

	*2.设置环境变量
		.bashrc 里添加
		export PATH=$PATH:/usr/local/storm/bin/
		export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/storm/lib/

	*3.测试：
		cd build
		cp ../config/*.conf ./
		1.启动Server
		  ./Server -f server.conf
		2.启动Client
		  ./Client  -f client.conf

#计划
	一些util类的实现
	1.sql拼接，看个几个开源项目的封装
	2.http parser
	3.log 类

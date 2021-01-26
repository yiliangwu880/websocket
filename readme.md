# 介绍：
	websocket 协议实现。
	独立于网络层。让你可以自由接入不同的网络层。
	高效
	
# 编译方法：
	整个文件夹放到linux目录，安装cmake gcc等。
	当前目录执行：sh clearBuild.sh 完成编译

# vs浏览代码：
	执行vs\run.bat,生成sln文件


目录结构：
	src						 ==源码
	include		             ==用户用的头文件
	bin		             	 ==执行文件
	lib                      ==用户用的静态库
	samples					 ==使用例子
	test					 ==测试用例
	vs                       ==vs浏览工具
	
	
	
详细功能说明：
	以include目录分类：
	
	static_trick		怪异但好用的静态编程。
	time				时间，定时器相关的功能。
	BacktraceInfo.h		崩溃时堆栈日志
	。。。





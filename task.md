




C++11 时间

日志分TRACE，普通文件。 TRACE输出文件包含所有的日志，普通的，按是否屏蔽一些级别来输出。
日志文件，过大就创建新的。

列表找最大或最小值，返回索引。 注意最小值可能由相同的多个

参考 delay_handler.h ，写异步调用，保存上下文
实现 守护进程 参考 man daemon


# 实时排行榜：
方法1：
{	
	mutimap<score, id>.  排序
	vector 存ID, 名次查找ID
	obj id:
	{
		score  . score查找map元素
	}
}
方法2：
{
	obj
	{
		id
		score
	}
	vector<obj>. 
	名次查找ID
	分数变化， 二分查找删除插入，并移动影响的元素。
	
}

# app管理器
 单例每个mgr,
 必须有event mgr
 启动进程mgr.
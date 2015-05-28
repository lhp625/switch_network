#! /bin/sh

host_dir=`echo ~`                                       # 当前用户根目录
proc_name="switch_pro"                                  # 进程名
file_name="/mnt/switch2/src/switch_debug.log"           # 日志文件
pid=0

proc_num()                                              # 计算进程数
{
	num=`ps -ef | grep $proc_name | grep -v grep | wc -l`
	#echo $num
	return $num
}

proc_id()                                               # 进程号
{
	pid=`ps -ef | grep $proc_name | grep -v grep | awk '{print $2}'`
}

proc_num
number=$?

echo "proc_num:$number ?= $num"

if [ $number -eq 0 ]                                    # 判断进程是否存在
then 
	cd $host_dir/mnt/switch2/src/; 
	#(./switch_pro &> ../../test.txt &)                 # 重启进程的命令，请相应修改
	nohup ./switch_pro &> ../test/test.txt &
	proc_id                                             # 获取新进程号
	echo ${pid}, `date` >> $host_dir$file_name          # 将新进程号和重启时间记录
fi


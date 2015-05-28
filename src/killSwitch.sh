#! /bin/sh

host_dir=`echo ~`                                       # ��ǰ�û���Ŀ¼
proc_name="switch_pro"                                  # ������
file_name="/mnt/switch2/src/switch_debug.log"           # ��־�ļ�

proc_id()                                               # ���������
{
	pid=`ps -ef | grep $proc_name | grep -v grep | awk '{print $2}'`
}

proc_id
proId=$?

echo "proc_id:proId ?= $pid"

if [ $pid -eq 0 ] 
then
	cd $host_dir/mnt/switch2/src/;
	kill -9 $pid;
	#nohup ./switch_pro &> ../test/test.txt &
	echo ${pid}, `date` >> $host_dir$file_name          # ���½��̺ź�����ʱ���¼
fi

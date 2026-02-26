#include "thread_algorithm.h"

thread_algorithm::thread_algorithm(QString name, QObject* parent)
	:thread_base(name, parent)
{

}

void thread_algorithm::process_task(const QVariant& task_data)
{
	QJsonObject task = task_data.toJsonObject();
	qDebug() << "[算法线程] 处理任务:" << task["command"];
	QThread::sleep(1); // 模拟耗时
}
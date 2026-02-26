#include "thread_motion_control.h"

thread_motion_control::thread_motion_control(QString name, QObject* parent)
    :thread_base(name, parent)
{

}

void thread_motion_control::process_task(const QVariant& task_data)
{
    QJsonObject task = task_data.toJsonObject();
    qDebug() << "[运动控制线程] 处理任务:" << task["command"];
    QThread::sleep(1); // 模拟耗时
}

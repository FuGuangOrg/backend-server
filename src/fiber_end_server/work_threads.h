/***************************************************************
 * 子线程类，主要处理四类任务:
 * (1) 算法检测任务
 * (2) 相机设备连接状态检测任务
 * (3) 运动控制任务
 * (4) 其他任务
 * 为了最大化服务并行能力以及提高响应速度，所有任务均在子线程中执行
 ***************************************************************/

#pragma once
#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QWaitCondition>
#include <QJsonObject>
#include <QJsonArray>

#include "../device_enum/device_enum_factory.h"

enum TASK_TYPE
{
	TASK_TYPE_ANY = 0,                  //任意不需要状态管理的任务
};


////////////////////////////////////////////////////////////////////////////////////////////////
//线程基类
class thread_base : public QThread
{
    Q_OBJECT
public:
    thread_base(const QString& name, QObject* parent = nullptr);
    virtual ~thread_base() override;

    void add_task(const QVariant& task_data);
    void stop();
signals:
    void post_task_finished(const QVariant& task_data);

protected:
    void run() override;
    virtual void process_task(const QVariant& task_data) = 0;

private:
	TYPE_SDK m_sdk_type{ SDK_DVP2 };        //使用哪个 SDK 操作相机.设备操作线程和枚举线程需要使用相同的 SDK
    QString m_name;
    QQueue<QVariant> m_task_queue;
    QMutex m_mutex;
    QWaitCondition m_wait_condition;
    bool m_running;
};
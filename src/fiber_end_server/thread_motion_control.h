/********************
 * 运动控制线程
 ********************/
#pragma once
#include "work_threads.h"

class thread_motion_control : public thread_base
{
public:
    thread_motion_control(QString name,QObject* parent = nullptr);
protected:
    void process_task(const QVariant& task_data) override;
};
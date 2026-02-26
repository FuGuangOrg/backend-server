/********************
 * 算法线程
 ********************/
#pragma once

#include "work_threads.h"

class thread_algorithm : public thread_base
{
public:
    thread_algorithm(QString name,QObject* parent = nullptr);
protected:
    void process_task(const QVariant& task_data) override;
};

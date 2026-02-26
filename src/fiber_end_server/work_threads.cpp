#include "work_threads.h"

#include <QDebug>

////////////////////////////////////////////////////////////////////////////////////////////////
thread_base::thread_base(const QString& name, QObject* parent)
    : QThread(parent), m_name(name), m_running(true)
{

}

thread_base::~thread_base()
{
    stop();
    wait(); // 等待线程结束
}

void thread_base::add_task(const QVariant& task_data)
{
    QMutexLocker locker(&m_mutex);
    m_task_queue.enqueue(task_data);
    m_wait_condition.wakeOne();
}

void thread_base::stop()
{
    QMutexLocker locker(&m_mutex);
    m_running = false;
    m_wait_condition.wakeAll();
}

void thread_base::run()
{
    while (true)
    {
        QVariant task_data;
        {
            QMutexLocker locker(&m_mutex);
            if (!m_running && m_task_queue.isEmpty())
                break;

            if (m_task_queue.isEmpty())
                m_wait_condition.wait(&m_mutex);

            if (!m_task_queue.isEmpty())
                task_data = m_task_queue.dequeue();
            else
                continue;
        }
        process_task(task_data); // 子类具体处理
    }
}
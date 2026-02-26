#include "grab_worker.h"
#include <QDebug>
#include <QThread>
#include <QImage>

grab_worker::grab_worker(interface_camera* camera,QObject* parent) :
	QObject(parent), m_camera(camera), m_running(true)
{
	
}

grab_worker::~grab_worker()
{
	
}

void grab_worker::stop()
{
    m_running.store(false);
}

void grab_worker::process()
{
    while (m_running.load())
    {
	    if (m_paused.load())
	    {
			m_is_paused.store(true);
            QThread::msleep(10);
            continue;
	    }
		m_is_paused.store(false);
        if (m_camera != nullptr) 
        {
            QImage img = m_camera->get_image(1000);
            emit frame_captured(img);
        }
        QThread::msleep(30); // 模拟采图间隔
    }
    m_finished.store(true);
    QThread::currentThread()->quit();  // 主动退出事件循环
}
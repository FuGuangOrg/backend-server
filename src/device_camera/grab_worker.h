/*************************************
 * 子线程对象，用于取图
 *************************************/

#pragma once

#pragma once
#include <QObject>
#include <atomic>
#include "interface_camera.h"

class grab_worker : public QObject
{
    Q_OBJECT

public:
    explicit grab_worker(interface_camera* camera,QObject* parent = nullptr);
    ~grab_worker();

	void stop();                                               //外部控制，停止采图并退出
	bool is_finished() const { return m_finished.load(); }     //外部访问，是否已经退出

	void pause() { m_paused.store(true); }              //外部控制，暂停采图
    bool is_paused() const { return m_is_paused.load(); }      //外部访问，是否已经暂停
	void resume() { m_paused.store(false); }            //外部控制，恢复采图
	

signals:
    void frame_captured(const QImage& img);
    void finished();

public slots:
    void process();

private:
    interface_camera* m_camera{ nullptr };
    std::atomic<bool> m_running{ false };       //正在执行
	std::atomic<bool> m_finished{ false };      //是否执行完毕
    std::atomic<bool> m_paused{ false };        //外部控制的暂停标志
	std::atomic<bool> m_is_paused{ false };     //是否已经暂停. 外部暂停后，线程函数可能仍在采图，存在一定延迟
};
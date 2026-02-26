#pragma once

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <queue>

#include <asio.hpp>
#include <asio/serial_port.hpp>

#include "motion_control.h"

#include <QObject>
#include <QJsonObject>

struct MOTION_CONTROL_EXPORT motion_parameter_port : public motion_parameter
{
    motion_parameter_port(const std::string& port_name, unsigned int baud_rate):
        m_port_name(port_name), m_baud_rate(baud_rate)
    {
	    
    }

    virtual ~motion_parameter_port() override = default;

    std::string m_port_name{ "" };
    unsigned int m_baud_rate{ 0 };
};

class MOTION_CONTROL_EXPORT motion_control_port :public motion_control
{
    Q_OBJECT
public:
    motion_control_port() = default;

    virtual ~motion_control_port() override;

    virtual bool initialize(motion_parameter* parameter) override;	//初始化参数
    virtual bool open() override;                                   //打开串口

    void async_read_in_thread();    //线程函数，异步读取硬件触发的消息
    void async_read();              //异步读取，实时读取硬件触发的消息
    void handle_async_read(const asio::error_code& ec, std::size_t bytes_transferred);

    virtual bool set_light_source_param(int frequency, int duty_cycle, int timeout = 1) override;

    virtual bool move_distance(int axis, int distance, int speed, int interval = 0) override;

    virtual bool move_position(int axis, int position, int speed, int interval = 0) override;

    virtual bool get_position(int& x, int& y, int& z) override;

    virtual bool reset(int axis) override;

    virtual bool set_current_position_zero(int axis) override;

    /*************************************************
    * 向串口发送命令,这里会阻塞当前线程，直到接收到设备返回的完整消息(设备执行完毕)或者超时
    * 如果用户没有设置超时时间，或者设置了无效的超时时间(<=0),超时时间默认为100秒
    * 正常时间内返回 true, 表示设备执行完毕;超时后返回 false, 表示设备可能出现异常
    **************************************************/
    bool send_command(const std::string& cmd, int timeout = 100, std::string* reply = nullptr);

    /*************************************************
    * 读取设备返回的消息，如果消息结尾字符为 finifh_ch 表示读取完毕，返回true
    * 否则表示没有读取完毕，返回 false
    ***********************************************/
    bool read_reply(std::string& reply, char finish_ch);

    /*************************************************
    * 串口如果由本线程打开，可以关闭
    ***********************************************/
    void close();

    bool is_opened() const { return m_is_opened; }

private:
    std::unique_ptr<asio::io_context> m_io;
    std::unique_ptr<asio::serial_port> m_serial;
    std::string m_port_name{ "" };
    unsigned int m_baud_rate{ 0 };
    bool m_is_opened{ false };

    std::thread m_async_thread;                          //子线程，实时检测外部(硬件)消息
    std::string m_async_buffer{ "" };               //存储设备返回消息的缓存
    std::chrono::steady_clock::time_point m_last_time;   //上一次硬件触发的时间
    int m_interval_time_ms{ 1000 };                      //两次硬件触发之间的时间间隔,单位ms

    std::mutex m_reply_mutex;                           // 命令回复
    std::condition_variable m_reply_cv;
    std::queue<std::string> m_reply_queue;
};
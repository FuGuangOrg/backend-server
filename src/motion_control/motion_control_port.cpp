#include "motion_control_port.h"
#include <asio/steady_timer.hpp>
#include "../common/common.h"

motion_control_port::~motion_control_port()
{
    if (m_serial && m_serial->is_open())
    {
        m_serial->close();
    }
    if (m_io)
    {
        m_io->stop();
    }
    if (m_async_thread.joinable())
    {
        m_async_thread.join();
    }
}

bool motion_control_port::initialize(motion_parameter* parameter)
{
    motion_parameter_port* parameter_port = dynamic_cast<motion_parameter_port*>(parameter);
    if(parameter_port == nullptr)
    {
        return false;
    }
    m_io = std::make_unique<asio::io_context>();
    m_serial = std::make_unique<asio::serial_port>(*m_io);
    m_port_name = parameter_port->m_port_name;
    m_baud_rate = parameter_port->m_baud_rate;
    return true;
}

bool motion_control_port::open()
{
    asio::error_code error_code;
    m_serial->close(error_code);           // 先关闭才能正常打开...
    m_serial->open(m_port_name, error_code);
    if (error_code)
    {
        write_log("open_port fail!");
        return false;
    }
    try
    {
        m_serial->set_option(asio::serial_port_base::baud_rate(m_baud_rate));
        m_serial->set_option(asio::serial_port_base::character_size(8));
        m_serial->set_option(asio::serial_port_base::parity(asio::serial_port_base::parity::none));
        m_serial->set_option(asio::serial_port_base::stop_bits(asio::serial_port_base::stop_bits::one));
        m_serial->set_option(asio::serial_port_base::flow_control(asio::serial_port_base::flow_control::none));
    }
    catch (const std::system_error& e)
    {
        m_serial->close();
        std::string info = std::string("set port config fail: ") + std::string(e.what());
        write_log(info.c_str());
        return false;
    }
    m_is_opened = true;
    //添加异步读取，实时检测硬件触发的消息
    async_read();
    // 启动 io_context 的事件循环线程
    m_async_thread = std::thread(&motion_control_port::async_read_in_thread, this);
    m_last_time = std::chrono::high_resolution_clock::now();
    return true;
}

void motion_control_port::async_read_in_thread()
{
    try
    {
        m_io->run();
    }
    catch (const std::exception& e)
    {
        // 日志或错误处理
        std::string info = std::string("async_read_in_thread error: ") + std::string(e.what());
        write_log(info.c_str());
    }
}

void motion_control_port::async_read()
{
    asio::async_read_until(
        *m_serial,
        asio::dynamic_buffer(m_async_buffer),
        '\n',
        std::bind(&motion_control_port::handle_async_read, this,
            std::placeholders::_1,
            std::placeholders::_2));
}

void motion_control_port::handle_async_read(const asio::error_code& ec, std::size_t bytes_transferred)
{
    if (!ec)
    {
        std::string msg = m_async_buffer.substr(0, bytes_transferred);
        m_async_buffer.erase(0, bytes_transferred);
        if (!msg.empty() && msg.back() == '\n')
        {
            msg.pop_back();
        }
        if (msg.rfind("device_", 0) != 0)       //msg 不是以 "device_" 开头
        {
            {
                std::lock_guard<std::mutex> lock(m_reply_mutex);
                m_reply_queue.push(msg);
            }
            m_reply_cv.notify_one();
        }
        else
        {
            //msg 以 "device_" 开头,表示开始检测或者更新坐标
            if(msg == "device_request_process")
            {
                std::chrono::steady_clock::time_point now = std::chrono::high_resolution_clock::now();
                auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_time);
                if (duration_ms.count() >= m_interval_time_ms)
                {
                    m_last_time = now;
                    emit post_device_request_start_process();
                }
            }
            else if(msg == "device_change_position")
            {
                emit post_device_change_position();
            }
            
            
        }
        async_read();
    }
}

bool motion_control_port::set_light_source_param(int frequency, int duty_cycle, int timeout)
{
    std::string cmd = "SetLight " + std::to_string(frequency) + " " + std::to_string(duty_cycle);
    return send_command(cmd, timeout);
}

bool motion_control_port::move_distance(int axis, int distance, int speed, int interval)
{
    if (axis != 0 && axis != 1 && axis != 2)
    {
        return false;
    }
    std::string cmd = "MoveDistance " + std::to_string(axis) + " " + std::to_string(distance) + " " + std::to_string(speed) + " " + std::to_string(interval);
    return send_command(cmd);
}

bool motion_control_port::move_position(int axis, int position, int speed, int interval)
{
    if (axis != 0 && axis != 1 && axis != 2)
    {
        return false;
    }
    std::string cmd = "MovePosition " + std::to_string(axis) + " " + std::to_string(position) + " " + std::to_string(speed) + " " + std::to_string(interval);
    return send_command(cmd);
}

bool motion_control_port::get_position(int& x, int& y, int& z)
{
    std::string cmd = "GetPosition";
    std::string reply("");
    if (send_command(cmd, 10, &reply))
    {
        size_t pos = reply.find_first_of(' ');
        x = atoi(reply.substr(0, pos).c_str());
        std::string sub = reply.substr(pos + 1);
        pos = sub.find_first_of(' ');
        y = atoi(sub.substr(0, pos).c_str());
        z = atoi(sub.substr(pos + 1).c_str());
    }
    return true;
}

bool motion_control_port::reset(int axis)
{
    if (axis != 0 && axis != 1 && axis != 2)
    {
        return false;
    }
    move_distance(axis, -100000, 20000);      //再向后移动到限制位
    move_distance(axis, 100, 2000);        //先向前移动一小段距离
    move_distance(axis, -200, 2000);        //先向前移动一小段距离
    //move_distance(axis, -100000, 3000);      //再向后移动到限制位
    return set_current_position_zero(axis);               //将当前位置设置为零点
}

bool motion_control_port::set_current_position_zero(int axis)
{
    if (axis != 0 && axis != 1 && axis != 2)
    {
        return false;
    }
    std::string cmd = "SetZero " + std::to_string(axis);
    return send_command(cmd);
}

void motion_control_port::close()
{
    try
    {
        if (m_serial->is_open())
        {
            m_serial->close();
        }
        // 检查是否成功关闭串口
        if (!m_serial->is_open())
        {
            m_is_opened = false;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "关闭串口时发生异常: " << e.what() << std::endl;
    }
}

bool motion_control_port::send_command(const std::string& cmd, int timeout, std::string* reply)
{
    if (timeout <= 0)
    {
        timeout = 100;
    }
    // 发送命令
    asio::write(*m_serial, asio::buffer(cmd));

    std::unique_lock<std::mutex> lock(m_reply_mutex);
    if (m_reply_cv.wait_for(lock, std::chrono::seconds(timeout),
        [this]() { return !m_reply_queue.empty(); }))
    {
        if (reply != nullptr)
        {
            *reply = m_reply_queue.front();
        }
        m_reply_queue.pop();
        return true;
    }
    return false; // 超时
}

bool motion_control_port::read_reply(std::string& reply, char finish_ch)
{
    using namespace asio;
    char sz_buf[256] = { 0 };
    asio::error_code error_code;
    size_t n(0);
    if (1)
    {
        //在后端控制脚本没有启动的情况下这里会阻塞程序，使用超时方案
        n = m_serial->read_some(buffer(sz_buf), error_code);
    }
    else
    {
        //超时处理
    }
    if (error_code)     //发生了错误
    {
        reply = "error";
        return true;
    }
    if (n > 0)
    {
        reply.append(sz_buf, n);
        if (reply.back() == finish_ch)
        {
            //std::cout << reply << std::endl;
            return true;
        }
    }
    return false;
}
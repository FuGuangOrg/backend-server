#pragma once

#include <mutex>
#include <queue>

#include <asio.hpp>
#include <asio/serial_port.hpp>
#include "modbus/modbus-tcp.h"

#include "motion_control.h"

struct MOTION_CONTROL_EXPORT motion_parameter_plc : public motion_parameter
{
    motion_parameter_plc(const std::string& ip_address, int port, const std::string& port_name, unsigned int baud_rate) :
        m_ip_address(ip_address), m_port(port),m_port_name(port_name),m_baud_rate(baud_rate)
    {

    }

    virtual ~motion_parameter_plc() override = default;

    std::string m_ip_address{ "" };
	int m_port{ 0 };
    std::string m_port_name{ "" };
    unsigned int m_baud_rate{ 0 };
};

class MOTION_CONTROL_EXPORT motion_control_plc :public motion_control
{
    Q_OBJECT
public:
    motion_control_plc() = default;

    virtual ~motion_control_plc() override;

    virtual bool initialize(motion_parameter* parameter) override;	//初始化参数
    virtual bool open() override;                                   //打开

    virtual bool set_light_source_param(int frequency, int duty_cycle, int timeout = 1) override;

    virtual bool move_distance(int axis, int distance, int speed, int interval = 0) override;

    virtual bool move_position(int axis, int position, int speed, int interval = 0) override;

    virtual bool get_position(int& x, int& y, int& z) override;

    virtual bool reset(int axis) override;

    virtual bool set_current_position_zero(int axis) override;

    //向串口发送命令，设置光源,不需要阻塞、超时机制，直接发送即可
    bool send_command_to_port(const std::string& cmd);
    //向 plc 发送命令，设置运动
    bool send_command_to_plc(const std::string& cmd, int timeout = 10, std::string* reply = nullptr);

    //线程函数，监视开关状态并发送消息执行检测功能
    void async_read_in_thread();
    //监视开关状态，这里参数固定为 4. 线程中运行，当开关闭合时返回1，否则返回0
    bool read_status(int x = 4);
private:
    //运动接口.该接口负责写入参数, 不负责执行实际运动
    //id:控制运动具体参数: 
    //304--沿X轴运动一段距离 1000--运动到X轴指定位置 310--沿X轴的运动速度
    //314--沿Y轴运动一段距离 1010--运动到Y轴指定位置 320--沿Y轴的运动速度
    //value:传入的参数值，可以是距离或者速度
    bool HD(int id, int value);

    //执行实际运动操作(需要将第二个变量置为true)
    //id:控制运动具体参数: 
    //610--沿第一个轴运动一段距离 1000--运动到第一个轴指定位置
    //611--沿第二个轴运动一段距离 1001--运动到第二个轴指定位置
    //o: 运动标识 true--执行运动 false--可以理解成复位
    //每次运动(调用M(id,true))之后需要调用M(id,false)复位
    bool M(int id, bool o = false);

    //检测轴是否在运动中. id: 1000 -- 第一个轴  1020 -- 第二个轴
    //如果在运动返回true,否则返回false
    bool SM(int id);

    //获取位置 id: 0 -- 第一个轴  4 -- 第二个轴
    int HSD(int id);

	static void convert_int_to_registers(int value, uint16_t* regs);

    std::string m_ip_address{ "" };
	int m_port{ 0 };
    std::string m_port_name{ "" };
    unsigned int m_baud_rate{ 0 };

    bool m_is_opened{ false };
	std::unique_ptr<asio::io_context> m_io;
    std::unique_ptr<asio::serial_port> m_serial;
    modbus_t* m_modbus_ctx{ nullptr };
    std::mutex m_plc_register_mutex;

    std::thread m_async_thread;                          //子线程，实时检测外部(硬件)消息
    std::chrono::steady_clock::time_point m_last_time;   //上一次硬件触发的时间
    int m_interval_time_ms{ 1000 };                      //两次硬件触发之间的时间间隔,单位ms
    std::atomic<bool> m_stop_thread{ false };     //子线程停止标识

    std::mutex m_reply_mutex;                           // 串口命令回复
    std::condition_variable m_reply_cv;
    std::queue<std::string> m_reply_queue;
};
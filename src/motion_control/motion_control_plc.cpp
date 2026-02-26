#include "motion_control_plc.h"
#include "modbus/modbus-tcp.h"
#include "../common/common.h"


motion_control_plc::~motion_control_plc()
{
    m_stop_thread.store(true);
    if (m_async_thread.joinable())
    {
        m_async_thread.join();  // 等待线程退出
    }
    if (m_modbus_ctx != nullptr)
    {
        modbus_free(m_modbus_ctx);
        m_modbus_ctx = nullptr;
    }
}

bool motion_control_plc::initialize(motion_parameter* parameter)
{
    motion_parameter_plc* parameter_plc = dynamic_cast<motion_parameter_plc*>(parameter);
    if (parameter_plc == nullptr)
    {
        return false;
    }
    m_ip_address = parameter_plc->m_ip_address;
    m_port = parameter_plc->m_port;

    m_port_name = parameter_plc->m_port_name;
    m_baud_rate = parameter_plc->m_baud_rate;
    m_io = std::make_unique<boost::asio::io_context>();
    m_serial = std::make_unique<boost::asio::serial_port>(*m_io);
    return true;
}

bool motion_control_plc::open()
{
    //打开串口，控制光源
    boost::system::error_code error_code;
    m_serial->close(error_code);           // 先关闭才能正常打开...
    m_serial->open(m_port_name, error_code);
    if (error_code)
    {
        write_log("open_port fail!");
        return false;
    }
    try
    {
        m_serial->set_option(boost::asio::serial_port_base::baud_rate(m_baud_rate));
        m_serial->set_option(boost::asio::serial_port_base::character_size(8));
        m_serial->set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
        m_serial->set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
        m_serial->set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));
    }
    catch (const boost::system::system_error& e)
    {
        m_serial->close();
        std::string info = std::string("set port config fail: ") + std::string(e.what());
        write_log(info.c_str());
        return false;
    }
    //连接ip，控制运控
    if(m_modbus_ctx != nullptr)
    {
        modbus_free(m_modbus_ctx);
        m_modbus_ctx = nullptr;
    }
    m_modbus_ctx = modbus_new_tcp(m_ip_address.c_str(), m_port);
    if (!m_modbus_ctx) 
    {
        write_log("Unable to create modbus context!");
        return false;
    }
    if (modbus_connect(m_modbus_ctx) == -1)
    {
        std::string sinfo = std::string("Connection failed: ") + std::string(modbus_strerror(errno));
        write_log(sinfo.c_str());
        modbus_free(m_modbus_ctx);
        m_modbus_ctx = nullptr;
        return false;
    }
    //启动子线程监视开关状态
    m_async_thread = std::thread(&motion_control_plc::async_read_in_thread, this);
    m_last_time = std::chrono::high_resolution_clock::now();
    return true;
}

bool motion_control_plc::set_light_source_param(int frequency, int duty_cycle, int timeout)
{
    std::string cmd = std::to_string(frequency) + " " + std::to_string(duty_cycle);
    return send_command_to_port(cmd);
}

bool motion_control_plc::move_distance(int axis, int distance, int speed, int interval)
{
    if (axis != 0 && axis != 1)
    {
        return false;
    }
    if(axis == 1)
    {
        HD(310, speed);     //先设置速度
        HD(304, distance);

        M(610, true);
    	M(610);
    }
    else
    {
        HD(320, speed);     //先设置速度
        HD(314, distance);
        M(611, true);
        M(611);
    }
    return true;
}

bool motion_control_plc::move_position(int axis, int position, int speed, int interval)
{
    if (axis != 0 && axis != 1)
    {
        return false;
    }
    if (axis == 1)   //1 -- X轴   0 -- Y轴
    {
        HD(310, speed);     //先设置速度
        HD(1000, position);
        M(1000, true);
        M(1000);
    }
    else
    {
        HD(320, speed);     //先设置速度
        HD(1010, position);
        M(1001, true);
        M(1001);
    }
    return true;
}

bool motion_control_plc::get_position(int& x, int& y, int& z)
{
    std::lock_guard<std::mutex> lock(m_plc_register_mutex);
    x = HSD(0);
    y = 0;
	z = HSD(4);
    return true;
}

bool motion_control_plc::reset(int axis)
{
    if (axis != 0)
    {
        return true;
    }
    if (!M(100, true))
        return false;
    M(100);
    return true;
}

bool motion_control_plc::set_current_position_zero(int axis)
{
    return true;
}

bool motion_control_plc::send_command_to_port(const std::string& cmd)
{
    // 发送命令
    boost::asio::write(*m_serial, boost::asio::buffer(cmd));
    return true;
}

bool motion_control_plc::send_command_to_plc(const std::string& cmd, int timeout, std::string* reply)
{
    if (!m_modbus_ctx) 
    {
        return false;
    }
    //int commandCode = encodeCommand(cmd);
    int commandCode(0);
    if (commandCode < 0)    //未知命令
    {
        return false;
    }
    // 写命令寄存器
    int ret = modbus_write_register(m_modbus_ctx, 0x0000, commandCode);
    if (ret == -1)
    {
        std::string sinfo = std::string("Failed to write command: ") + std::string(modbus_strerror(errno));
        return false;
    }
    // 等待执行完成
    uint16_t status = 0;
    while (true) 
    {
        ret = modbus_read_registers(m_modbus_ctx, 0x0001, 1, &status);
        if (ret == -1) 
        {
            std::string sinfo = std::string("Failed to read status: ") + std::string(modbus_strerror(errno));
            write_log(sinfo.c_str());
            return false;
        }
        if (status == 2)    // 完成
        { 
            return true;
        }
        else if (status == 3) // 错误
        { 
            write_log("Controller reported error");
            return false;
        }
        // 睡眠一会儿再查，避免占满 CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return true;
}

bool motion_control_plc::M(int id, bool o)
{
    {
        std::lock_guard<std::mutex> lock(m_plc_register_mutex);
        int bit = o ? 1 : 0;
        int rc = modbus_write_bit(m_modbus_ctx, id, bit);
        if (rc == -1)
        {
            std::string sinfo = std::string("Write coil failed: ") + std::string(modbus_strerror(errno));
            write_log(sinfo.c_str());
            return false;
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    //如果是移动，需要阻塞，等待移动完毕才返回
    if(o)
    {
        int axis = 0;
        if (id == 610 || id == 1000)
        {
            axis = 1000;
        }
        else if(id == 611 || id == 1001)
        {
            axis = 1020;
        }
        else if(id == 100)  //复位时需要等两个轴都停止
        {
            axis = 2020;
        }
        if(axis == 0)
        {
            return true;
        }
        auto start = std::chrono::steady_clock::now();
	    while (true)
	    {
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() > 10)
            {
	            break;
            }
            if(axis == 2020)
            {
                if (SM(axis-1000) || SM(axis - 1020))
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
            }
            else
            {
                if (SM(axis))
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
            }
            break;
	    }
    }
    return true;
}

bool motion_control_plc::HD(int id, int value)
{
    std::lock_guard<std::mutex> lk(m_plc_register_mutex);
    uint16_t regs[2];
    convert_int_to_registers(value, regs);
    int rc = modbus_write_registers(m_modbus_ctx, 41088 + id, 2, regs);
    if (rc == -1)
    {
        std::string info = std::string("Write registers failed: ") + std::string(modbus_strerror(errno));
        write_log(info.c_str());
        return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    return true;
}

int motion_control_plc::HSD(int id)
{
    uint16_t regs[2] = { 0 };
    int rc = modbus_read_input_registers(m_modbus_ctx, 47232+id, 2, regs);
    if (rc == -1)
    {
        std::string sinfo = std::string("Read input registers failed: ") + std::string(modbus_strerror(errno));
        write_log(sinfo.c_str());
        return 0;
    }
    int ret = (static_cast<int>(regs[1]) << 16) | regs[0];
    return ret;
}

bool motion_control_plc::SM(int id)
{
    try
    {
        std::lock_guard<std::mutex> lock(m_plc_register_mutex);
        uint8_t coil_status = 0;                                 // 存储读取结果
        int rc = modbus_read_bits(m_modbus_ctx, 36864 + id, 1, &coil_status);
        if (rc == -1)
        {
            std::string sinfo = std::string("Read coil failed: ") + std::string(modbus_strerror(errno));
            write_log(sinfo.c_str());
            return false;
        }
        return coil_status != 0;
    }
    catch (...)
    {
        return false;  // 出现任何异常时返回 false
    }
}

void motion_control_plc::convert_int_to_registers(int value, uint16_t* regs)
{
    regs[1] = static_cast<uint16_t>((value >> 16) & 0xFFFF); // 高位
    regs[0] = static_cast<uint16_t>(value & 0xFFFF);         // 低位
}

void motion_control_plc::async_read_in_thread()
{
	while (true)
	{
		if(m_stop_thread.load())
		{
			break;
		}
        if(read_status())
        {
            std::chrono::steady_clock::time_point now = std::chrono::high_resolution_clock::now();
            auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_time);
            if (duration_ms.count() >= m_interval_time_ms)
            {
                m_last_time = now;
                emit post_device_request_start_process();
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

bool motion_control_plc::read_status(int x)
{
    std::lock_guard<std::mutex> lock(m_plc_register_mutex);
    try
    {
        uint8_t coil_status = 0;
        int rc = modbus_read_bits(m_modbus_ctx, 20480 + x, 1, &coil_status);
        if (rc == -1)
        {
            std::string sinfo = std::string("Read coil failed: ") + std::string(modbus_strerror(errno));
            write_log(sinfo.c_str());
            return false;
        }

        return coil_status != 0;
    }
    catch (...)
    {
        return false;
    }
}
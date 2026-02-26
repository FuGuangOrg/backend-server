#pragma once

#include "motion_control_global.h"

#include <QObject>


//初始化运控对象时使用的参数
struct MOTION_CONTROL_EXPORT motion_parameter
{
	motion_parameter() = default;
	virtual ~motion_parameter() = default;
};

class MOTION_CONTROL_EXPORT motion_control : public QObject
{
	Q_OBJECT
public:
	motion_control() = default;
	virtual ~motion_control() = default;

	virtual bool initialize(motion_parameter* parameter) = 0;	//初始化参数

	virtual bool open() = 0;		//通过串口或者TCP或其他方式打开控制设备,在 initialize 之后调用

	/*************************************************
	* 设置光源亮度
	* frequency     -- 光源频率，单位时间内的周期数（例如：赫兹，Hz）。频率越高，光源的闪烁周期越短。(可以)固定为 1000000
	* duty_cycle    -- 占空比。光源在一个周期内的开启时间比例，通常用百分比表示。比如，80% 的占空比表示光源在周期内的 80% 时间是开启的，剩余 20% 时间是关闭的
	* timeout_s     -- 判断超时。如果发送命令之后 timeout 秒内仍然没有返回消息，表示超时
	*************************************************/
	virtual bool set_light_source_param(int frequency, int duty_cycle, int timeout = 1) = 0;

	/*************************************************
	* 沿指定轴移动一定距离.
	* axis      --  指定移动的轴. 0 : X轴  1 : Y轴
	* distance  --  移动的距离大小，脉冲数量
	* speed     --  移动速度
	*************************************************/
	virtual bool move_distance(int axis, int distance, int speed, int interval = 0) = 0;

	/*************************************************
	* 沿指定轴移动到指定位置.
	* axis      --  指定移动的轴. 0 : X轴  1 : Y轴
	* position  --  目的地在轴上的位置
	* speed     --  移动速度
	*************************************************/
	virtual bool move_position(int axis, int position, int speed, int interval = 0) = 0;

	/*************************************************
	* 获取位置
	*************************************************/
	virtual bool get_position(int& x, int& y,int& z) = 0;

	/*************************************************
   * 重置位置,沿指定轴移动到负限位并设置为零点
   * 首先往前移动一小段距离，然后往后移动到限制位，再将限制位设置为零点
   * 重置之后,该轴上的坐标范围为[0,N],单位为脉冲。
   * 之所以需要重置，是因为每个轴上的零点可以是任意位置，如果将某个位置设为零点，则其范围成为[-M,N]
   * 重置是为了将该轴上所有点坐标设为正值，便于处理
   * axis:  0--X轴 1--Y轴
   *************************************************/
	virtual bool reset(int axis) = 0;

	/************************************************
	 * 将当前位置设为 0 点. 0 -- X 轴 1 -- Y 轴
	 ************************************************/
	virtual bool set_current_position_zero(int axis) = 0;

signals:
	void post_device_request_start_process();		//监视设备信号，执行检测
	void post_device_change_position();				//监视设备信号,修改坐标
};

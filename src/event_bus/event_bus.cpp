#include "event_bus.h"


event_bus::event_bus(QObject* parent)
	:QObject(parent)
{

}

event_bus::~event_bus()
{
	
}


void event_bus::publish(const QString& event, const QVariant& payload)
{
    if (!m_subscribers.contains(event))
        return;
    const auto& subs = m_subscribers[event];
    for (const auto& sub : subs) 
    {
        if (sub.receiver) 
        {
            qDebug() << "Invoking slot:" << sub.slot << "on" << sub.receiver;
            QMetaObject::invokeMethod(sub.receiver, sub.slot.constData(),
                Qt::QueuedConnection,
                Q_ARG(QVariant, payload));
        }
    }
}

void event_bus::subscribe(const QString& event, QObject* receiver, const char* slot)
{
    if (!receiver || !slot)
        return;
    
    QByteArray method = QByteArray(slot);
    if (method.length() > 1 && std::isdigit(static_cast<unsigned char>(method.at(0))))
        method.remove(0, 1);
    Subscriber sub{ receiver, method };
    if (!m_subscribers[event].contains(sub)) 
    {
        m_subscribers[event].append(sub);
    }
}

void event_bus::unsubscribe(const QString& event, QObject* receiver, const char* slot)
{
    if (!m_subscribers.contains(event))
        return;
    QByteArray method = QByteArray(slot);
    if (method.length() > 1 && std::isdigit(static_cast<unsigned char>(method.at(0))))
        method.remove(0, 1);
    Subscriber target;
    target.receiver = receiver;
    target.slot = method;
    // 移除匹配的 subscriber
    auto& subscribers = m_subscribers[event];
    subscribers.removeAll(target);
    // 如果该事件没有任何订阅者了，移除该事件项
    if (subscribers.isEmpty()) 
    {
        m_subscribers.remove(event);
    }
}
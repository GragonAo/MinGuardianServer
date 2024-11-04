#pragma once

#include <stdexcept> // 引入异常处理相关的头文件

// 定义一个单例模板类，模板参数为 T
template <typename T>
class Singleton {
public:
    // 获取单例实例的静态方法，支持传递参数以构造实例
    template<typename... Args>
    static T* Instance(Args&&... args) {
        // 如果实例尚未创建，则创建新的实例
        if (m_pInstance == nullptr)
            m_pInstance = new T(std::forward<Args>(args)...); // 完美转发参数

        return m_pInstance; // 返回单例实例
    }

    // 获取单例实例的静态方法，如果未初始化则抛出异常
    static T* GetInstance() {
        // 检查实例是否已创建
        if (m_pInstance == nullptr)
            throw std::logic_error("the instance is not init, please initialize the instance first");

        return m_pInstance; // 返回单例实例
    }

    // 销毁单例实例的静态方法
    static void DestroyInstance() {
        delete m_pInstance; // 删除实例
        m_pInstance = nullptr; // 将指针置空
    }

private:
    static T* m_pInstance; // 静态成员变量，用于存储单例实例的指针
};

// 初始化静态成员变量
template <class T> T* Singleton<T>::m_pInstance = nullptr;

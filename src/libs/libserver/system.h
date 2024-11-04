#pragma once
#include "disposable.h"
#include "common.h"
#include "sn_object.h"
class EntitySystem;

/**
 * @brief 基础系统类，用于定义系统的通用行为。
 * 
 * `System` 类继承自 `IDisposable`，具备可释放资源的能力。该类提供系统的基础接口，如 `Dispose`、`GetTypeName` 和 `Update` 方法。
 * 它是所有具体系统的基类。
 */
class System : public IDisposable, public SnObject
{
public:
    virtual ~System() = default;  // 虚析构函数，确保子类的正确析构

    /**
     * @brief 实现 IDisposable 接口的 Dispose 方法，释放系统资源。
     * 
     * 默认情况下不做任何操作，子类可以重写此方法。
     */
    void Dispose() override {}

    /**
     * @brief 获取系统类型的名称。
     * 
     * @return const char* 系统类型的名称，默认返回空字符串。
     */
    virtual const char* GetTypeName() { return ""; }

    /**
     * @brief 获取系统类型的哈希码，用于标识系统类型。
     * 
     * @return uint64 系统类型的哈希码，默认返回 0。
     */
    virtual uint64 GetTypeHashCode() { return 0; }

    /**
     * @brief 系统更新函数，供派生类重写。
     * 
     * @param pEntities 实体系统指针，允许系统在每帧更新时操作实体。
     */
    virtual void Update(EntitySystem* pEntities) {}
};

/**
 * @brief 泛型系统接口类，用于为具体系统类型提供通用实现。
 * 
 * `ISystem` 继承自 `System`，并使用模板参数 `T` 来获取系统类型。通过模板机制，该类实现了系统类型名称和哈希码的自动获取。
 * @tparam T 系统类型。
 */
template<class T>
class ISystem : public System
{
public:
    virtual ~ISystem() = default;  // 虚析构函数，确保子类的正确析构

    /**
     * @brief 获取系统类型名称。
     * 
     * 使用 `typeid` 获取模板参数 `T` 的名称。
     * 
     * @return const char* 系统类型的名称。
     */
    virtual const char* GetTypeName() override { return typeid(T).name(); }

    /**
     * @brief 获取系统类型的哈希码。
     * 
     * 使用 `typeid` 获取模板参数 `T` 的哈希码。
     * 
     * @return uint64 系统类型的哈希码。
     */
    virtual uint64 GetTypeHashCode() override { return typeid(T).hash_code(); }
};

/**
 * @brief 唤醒系统的基础类，继承自 IDisposable。
 * 
 * 该类定义了基础唤醒系统的行为，通常用于需要在创建时执行某些初始化操作的系统。
 */
class IAwakeSystemBase : public IDisposable
{
public:
    void Dispose() override {}  // 空实现，子类可以重写释放资源的逻辑

    /**
     * @brief 判断系统是否为单例。
     * 
     * 默认返回 true，表示该系统为单例。
     * 
     * @return bool 系统是否为单例。
     */
    static bool IsSingle() { return true; }
};

/**
 * @brief 唤醒系统接口类，用于定义带参数的唤醒行为。
 * 
 * 该模板类继承自 `IAwakeSystemBase`，并允许派生类通过 `Awake` 方法初始化系统。
 * @tparam TArgs 唤醒时的参数类型列表。
 */
template <typename... TArgs>
class IAwakeSystem : virtual public IAwakeSystemBase
{
public:
    IAwakeSystem() = default;  // 默认构造函数
    virtual ~IAwakeSystem() = default;  // 虚析构函数

    /**
     * @brief 唤醒系统，执行初始化操作。
     * 
     * 纯虚函数，需要派生类实现具体的唤醒逻辑。
     * @param args 唤醒时传递的参数。
     */
    virtual void Awake(TArgs... args) = 0;
};

/**
 * @brief 从对象池唤醒系统的基础类，继承自 IDisposable。
 * 
 * 该类定义了从对象池中唤醒的系统，通常用于需要重复使用的系统。
 */
class IAwakeFromPoolSystemBase : public IDisposable
{
public:
    void Dispose() override {}  // 空实现，子类可以重写释放资源的逻辑

    /**
     * @brief 判断系统是否为单例。
     * 
     * 默认返回 false，表示该系统不是单例，通常用于对象池中的系统。
     * 
     * @return bool 系统是否为单例。
     */
    static bool IsSingle() { return false; }
};

/**
 * @brief 从对象池唤醒系统接口类，用于定义带参数的唤醒行为。
 * 
 * 该模板类继承自 `IAwakeFromPoolSystemBase`，允许派生类通过 `Awake` 方法从对象池中唤醒并初始化系统。
 * @tparam TArgs 唤醒时的参数类型列表。
 */
template <typename... TArgs>
class IAwakeFromPoolSystem : virtual public IAwakeFromPoolSystemBase
{
public:
    IAwakeFromPoolSystem() = default;  // 默认构造函数
    virtual ~IAwakeFromPoolSystem() = default;  // 虚析构函数

    /**
     * @brief 从对象池唤醒系统，执行初始化操作。
     * 
     * 纯虚函数，需要派生类实现具体的唤醒逻辑。
     * @param args 唤醒时传递的参数。
     */
    virtual void Awake(TArgs... args) = 0;
};

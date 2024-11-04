#pragma once

#include <map>

// 宏定义，用于创建状态的工厂方法
#define DynamicStateCreate(classname, enumType) \
    static void* CreateState() { return new classname; } \
    RobotStateType GetState() override { return enumType; }

// 宏定义，用于绑定状态的创建方法
#define DynamicStateBind(classname) \
    reinterpret_cast<CreateIstancePt>( &( classname::CreateState ) )

// T 为父类类型的模板类，enumType 为状态类型的枚举
template<typename enumType, class T>
class StateTemplate {
public:
    // 构造函数，初始化父对象指针为空
    StateTemplate() {
        _pParentObj = nullptr;
    }

    // 设置父对象
    void SetParentObj(T* pObj) {
        _pParentObj = pObj;
    }

    // 虚析构函数
    virtual ~StateTemplate() { }

    // 纯虚函数，获取当前状态
    virtual enumType GetState() = 0;

    // 纯虚函数，更新状态
    virtual enumType Update() = 0;

    // 纯虚函数，进入状态
    virtual void EnterState() = 0;

    // 纯虚函数，离开状态
    virtual void LeaveState() = 0;

protected:
    T* _pParentObj{ nullptr }; // 父对象指针
};

// 状态管理类模板
template<typename enumType, class StateClass, class T>
class StateTemplateMgr {
public:
    // 析构函数，清理当前状态
    virtual ~StateTemplateMgr() {
        if (_pState != nullptr) {
            delete _pState;
        }
    }

    // 初始化状态管理器
    void InitStateTemplateMgr(enumType defaultState) {
        _defaultState = defaultState; // 设置默认状态
        RegisterState(); // 注册状态
    }

    // 切换状态
    void ChangeState(enumType stateType) {
        StateClass* pNewState = CreateStateObj(stateType); // 创建新状态
        if (pNewState == nullptr) {
            //LOG_ERROR( "ChangeState:" << stateType << " == nullptr" );
            return; // 如果新状态为空，返回
        }

        if (pNewState != nullptr) {
            if (_pState != nullptr) {
                _pState->LeaveState(); // 离开当前状态
                delete _pState; // 删除当前状态
            }

            _pState = pNewState; // 更新当前状态为新状态
            _pState->EnterState(); // 进入新状态
        }
    }

    // 更新当前状态
    void UpdateState() {
        if (_pState == nullptr) {
            ChangeState(_defaultState); // 如果当前状态为空，切换到默认状态
        }
        enumType curState = _pState->Update(); // 更新当前状态并获取状态
        if (curState != _pState->GetState()) {
            ChangeState(curState); // 如果当前状态与已知状态不同，切换状态
        }
    }

protected:
    virtual void RegisterState() = 0; // 纯虚函数，供子类实现状态注册

    ///////////////////////////////////////////////////////////////////////////////////
    // 状态创建和注册管理
public:
    typedef StateClass* (*CreateIstancePt)(); // 状态创建函数指针类型

    // 根据状态值创建状态对象
    StateClass* CreateStateObj(enumType enumValue) {
        auto iter = _dynCreateMap.find(enumValue); // 查找状态映射
        if (iter == _dynCreateMap.end())
            return nullptr; // 未找到返回空指针

        CreateIstancePt np = iter->second; // 获取创建实例的指针
        StateClass* pState = np(); // 调用创建实例的指针，创建状态对象
        pState->SetParentObj(static_cast<T*>(this)); // 设置父对象为当前状态管理器
        return pState; // 返回新创建的状态对象
    }

    // 注册状态类及其创建函数
    void RegisterStateClass(enumType enumValue, CreateIstancePt np) {
        _dynCreateMap[enumValue] = np; // 将状态值与创建函数映射
    }

    ///////////////////////////////////////////////////////////////////////////////////

protected:
    std::map<enumType, CreateIstancePt> _dynCreateMap; // 状态映射表
    StateClass* _pState{ nullptr }; // 当前状态对象指针
    enumType _defaultState; // 默认状态
};

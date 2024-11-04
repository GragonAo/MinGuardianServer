#pragma once

#include <string>
#include <map>
#include "resource_base.h"
#include "resource_mgr_template.h"
#include "libserver/vector3.h"

// 定义资源世界的类型
enum class ResourceWorldType
{
    Login = 1,     // 登录场景
    Roles = 2,     // 角色选择场景
    Public = 3,    // 公共场景
    Dungeon = 4,   // 地下城
};

// 世界资源类
class ResourceWorld : public ResourceBase
{
public:
    // 构造函数，接受一个包含头信息的映射
    explicit ResourceWorld(std::map<std::string, int>& head);

    // 检查资源是否有效
    bool Check() override;

    // 获取世界的名称
    std::string GetName() const;

    // 获取世界的类型
    ResourceWorldType GetType() const;

    // 检查该资源是否为指定类型
    bool IsType(ResourceWorldType iType) const;

    // 检查是否为初始化地图
    bool IsInitMap() const;

    // 获取初始化位置
    Vector3 GetInitPosition() const;

protected:
    // 生成结构体
    void GenStruct() override;

private:
    std::string _name{ "" };                   // 世界名称
    bool _isInit{ false };                      // 是否为初始化地图标识
    ResourceWorldType _worldType{ ResourceWorldType::Dungeon }; // 世界类型
    Vector3 _initPosition{ 0,0,0 };             // 初始化位置
};

// 世界资源管理类
class ResourceWorldMgr : public ResourceManagerTemplate<ResourceWorld>
{
public:
    // 初始化后的处理
    bool AfterInit() override;

    // 获取初始化地图
    ResourceWorld* GetInitMap();

    // 获取角色选择地图
    ResourceWorld* GetRolesMap();

private:
    int _initMapId{ 0 };   // 初始化地图的 ID
    int _rolesMapId{ 0 };  // 角色选择地图的 ID
};

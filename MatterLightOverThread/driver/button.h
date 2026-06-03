#pragma once

//#include <stdint.h>
//#include "AppEvent.h" // 必须引入原厂头文件，用于对齐原厂函数指针定义
//
///**
// * @brief 自定义按键事件结构体（严格保持原厂强转架构）
// */
//struct AppButtonEvent 
//{
//    enum AppButtonEventType
//    {
//        kEventType_None = 0,
//        kEventType_Button = 1, // 必须与原厂的 AppEvent::kEventType_Button 值完全一致
//    };
//
//    // ✉️ 自定义邮件里写的“具体动作”
//    enum AppButtonAction
//    {
//        kButtonAction_ShortPress = 1,   // 单击 (对齐 1)
//        kButtonAction_DoublePress,      // 双击 (对齐 2)
//        kButtonAction_LongPressStart,   // 长按开始触发 (对齐 3)
//        kButtonAction_LongPressing,     // 长按不放时的连续计数脉冲 (对齐 4)
//        kButtonAction_LongPressEnd      // 长按结束释放 (对齐 5)
//    };
//
//    uint16_t Type; // 填入 kEventType_Button
//
//    // 💡 核心修正：利用原厂必定存在且不会报错的 ButtonEvent.Action 字段来承载所有按键数据
//    // 我们把 ButtonIdx 放在低 8 位，Action 放在中 8 位，Count 放在高 16 位，完美避开原厂改名报错
//    union
//    {
//        struct
//        {
//            uint32_t Action; 
//        } ButtonEvent;
//    };
//
//    // 🎯 核心收信人：这里的入参固定为原厂的 AppEvent* 指针
//    typedef void (*EventHandler)(AppEvent *);
//    EventHandler Handler;
//};
//
//// ==================== 📢 外部函数接口声明 ====================
//
///**
// * @brief 初始化自定义按键服务的辅助定时器
// */
//void MyCustomButtonInit(void);
//
///**
// * @brief 自定义硬件中断回调函数（供 AppInit 中注册使用）
// */
//void MyCustomButtonInterruptHandler(uint8_t button, uint8_t btnAction);
//
///**
// * @brief 全局事件接收处理函数（异步消费队列中的按键信件）
// * @note  由于用户要求该函数在其他文件中，这里仅做 extern 声明，实体在你的业务文件中
// */
//extern void MyButtonActionHandler(AppEvent * aEvent);

#include <stdint.h>
#include "AppEvent.h" // 必须引入原厂头文件，用于对齐原厂函数指针定义

/**
 * @brief 自定义按键事件结构体（严格保持原厂强转架构与内存对齐）
 */
struct AppButtonEvent 
{
    enum AppButtonEventType
    {
        kEventType_None = 0,
        kEventType_Button = 1, // 必须与原厂的 AppEvent::kEventType_Button 值完全一致
    };

    // ✉️ 自定义邮件里写的“具体动作”
    enum AppButtonAction
    {
        kButtonAction_ShortPress = 0,   // 单击
        kButtonAction_DoublePress,      // 双击
        kButtonAction_LongPressStart,   // 长按开始触发
        kButtonAction_LongPressing,     // 长按不放时的连续计数脉冲
    };

    uint16_t Type; // 填入 kEventType_Button

    // 💡 核心修正：扁平化结构，直接定义成员，彻底消除匿名 union 在新版 GCC 中的编译红线
    struct
    {
        uint8_t ButtonIdx;       // 哪个按键
        uint8_t Action;          // 填入上面的 AppButtonAction 动作
        uint16_t LongPressCount; // 如果是长按，记录当前是第几次脉冲
    } ButtonEvent;

    // 🎯 核心收信人：这里的入参必须固定为原厂的 AppEvent* 指针
    typedef void (*EventHandler)(AppEvent *);
    EventHandler Handler;
};

// ==================== 📢 外部函数接口声明 ====================

/**
 * @brief 初始化自定义按键服务的辅助定时器
 */
void MyCustomButtonInit(void);

/**
 * @brief 自定义硬件中断回调函数（供 AppInit 中注册使用）
 */
void MyCustomButtonInterruptHandler(uint8_t button, uint8_t btnAction);

/**
 * @brief 全局事件接收处理函数（异步消费队列中的按键信件）
 */
void MyButtonActionHandler(AppEvent * aEvent);


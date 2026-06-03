#include "my_app.h"

#include "sl_udelay.h"
#include "AppTask.h"
#include "AppConfig.h"
#include "AppEvent.h"

#include "LEDWidget.h"
#if (defined(SL_MATTER_RGB_LED_ENABLED) && SL_MATTER_RGB_LED_ENABLED == 1)
#include "RGBLEDWidget.h"
#endif //(defined(SL_MATTER_RGB_LED_ENABLED) && SL_MATTER_RGB_LED_ENABLED == 1)

#include <app/persistence/AttributePersistenceProviderInstance.h>
#include <app/persistence/DefaultAttributePersistenceProvider.h>
#include <app/persistence/DeferredAttributePersistenceProvider.h>

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/clusters/on-off-server/on-off-server.h>
#include <app/server/Server.h>
#include <app/util/attribute-storage.h>
#include <setup_payload/OnboardingCodesUtil.h>

#include <assert.h>

#include <platform/silabs/platformAbstraction/SilabsPlatform.h>

#include <setup_payload/QRCodeSetupPayloadGenerator.h>
#include <setup_payload/SetupPayload.h>

#include <lib/support/CodeUtils.h>

#include <lib/support/Span.h>
#include <platform/CHIPDeviceLayer.h>
#include <platform/silabs/tracing/SilabsTracingMacros.h>


//加载 iadc
#include "../driver/iadc_driver.h"

uint32_t g_time_clk=0;
uint32_t g_time_detect_bat = 0;
bool g_is_operation_in_progress = false;//耗时过长的操作正在进行中

/**
 * @brief 📬 初始化阶段：在这里设置好所有的硬件和软件资源，准备好迎接后续的业务逻辑
 */
void my_custom_init_app_process(void) {

    // Your initialization code here
    SILABS_LOG("[app]run start");

    gpio_init(); // 初始化 GPIO 引脚
    SILABS_LOG("[app]gpio Init");
    MyCustomButtonInit(); // 初始化自定义按键服务 用于创建定时器和设置中断回调
    SILABS_LOG("[app]MyCustomButtonInit");

 
    powerManage_adc_Init();
    SILABS_LOG("[app]powerManage_adc_Init");
    PowerManageInit(); // 初始化电源管理
    SILABS_LOG("[app]Power completed");

}


/**
 * @brief 📬 主循环阶段：在这里处理所有的业务逻辑，包括电源管理、按键处理等
 */
void my_custom_loop_app_process(void) {
    static uint32_t tick = 0;
   //获取计时器的值
    g_time_clk++;

    //获取电源状态
    GetExternPowerFlag();

    //电源管理 用于电源状态的切换
    PowerSwitchAssignment();

    if(eg_PowerStatus == false)
    {
        //电池模式
        if(g_time_clk - g_time_detect_bat > 100)//1s 检测一次电池状态
        {
            g_time_detect_bat = g_time_clk;
            GetDisChargeStatus();
            SILABS_LOG("Battery Status=%d",eg_BatStatus);
            extern unsigned int g_ADBatLowVal;
            SILABS_LOG("BatVol=%d",g_ADBatLowVal);
            g_is_operation_in_progress=false;
        }
        BatOutEn();
    }
    else
    {
        //外部供电模式
        ChargeTimeUpdata();//充电时间更新
        BatOutDis();
        ChargeLogic(ChargeDetect());
        ChargeCurrentCtrlOut(false);
    }
    if(g_time_clk - tick > 200)
    {   
        tick = g_time_clk;
       // SILABS_LOG("tick=%d",tick);
    }
}


/**
 * @brief 📬 异步消费端：成功寄居在其他业务文件中
 */

void MyButtonActionHandler(AppEvent * aEvent)
{
    SILABS_LOG("【MyButtonActionHandler】Enter button event handler");
    
    // 💡 保持强转架构解包法：你怎么强转过去，我就怎么强转回来读，内存完美重合！
    AppButtonEvent* pBtnEvent = reinterpret_cast<AppButtonEvent*>(aEvent);
    
    uint8_t  button_idx       = pBtnEvent->ButtonEvent.ButtonIdx;
    uint8_t  action           = pBtnEvent->ButtonEvent.Action;
    uint16_t long_press_count = pBtnEvent->ButtonEvent.LongPressCount;

    switch (action)
    {
        case AppButtonEvent::kButtonAction_ShortPress:
            SILABS_LOG(" -> [业务确诊] 按键 %d : 单击触发！", button_idx);
            // 这里写你的单击控制代码
            break;
            
        case AppButtonEvent::kButtonAction_DoublePress:
            SILABS_LOG(" -> [业务确诊] 按键 %d : 双击触发！", button_idx);
            // 这里写你的双击控制代码
            break;
            
        case AppButtonEvent::kButtonAction_LongPressStart:
            SILABS_LOG(" -> [业务确诊] 按键 %d : 开始长按！", button_idx);
            break;
            
        case AppButtonEvent::kButtonAction_LongPressing:
            SILABS_LOG(" -> [业务确诊] 按键 %d : 长按持续中... 脉冲计数 = %d", button_idx, long_press_count);
            break;
            
        default:
            SILABS_LOG(" -> [未捕获] 动作编号: %d", action);
            break;
    }
}

#include "my_app.h"

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




void my_custom_init_app_process(void) {
    // Your initialization code here
    MyCustomButtonInit(); // 初始化自定义按键服务 用于创建定时器和设置中断回调
    SILABS_LOG("Custom app initialization completed");
    
}

void my_custom_loop_app_process(void) {
    // Your initialization code here
    //10ms执行一次这个函数，你可以在这个函数中添加你自己的代码
    SILABS_LOG("Running custom loop process");

}


/**
 * @brief 📬 异步消费端：专门用来执行按键业务的收信办公室
 */
__attribute__((weak)) void MyButtonActionHandler(AppEvent * aEvent)
{
    // 🎯 进来第一步：将原厂的标准信件强制逆向转换，还原为我们自定义的结构体指针
    AppButtonEvent* my_event = reinterpret_cast<AppButtonEvent*>(aEvent);

    if (my_event->Type != AppButtonEvent::kEventType_Button) return;

    uint8_t action = my_event->ButtonEvent.Action;
    uint16_t count = my_event->ButtonEvent.LongPressCount;

    switch (action)
    {
        case AppButtonEvent::kButtonAction_ShortPress:
            SILABS_LOG("【安全强转】收到邮件：单击！");
            // 在这里安全地执行你的业务，比如翻转 LED 状态： LightMgr().ToggleLight();
            break;
            
        case AppButtonEvent::kButtonAction_DoublePress:
            SILABS_LOG("【安全强转】收到邮件：双击！！");
            // 执行双击对应的业务
            break;
            
        case AppButtonEvent::kButtonAction_LongPressStart:
            SILABS_LOG("【安全强转】收到邮件：长按开始");
            break;
            
        case AppButtonEvent::kButtonAction_LongPressing:
            SILABS_LOG("【安全强转】收到邮件：长按持续中... 当前计数 = %d", count);
            // 比如长按不放时，根据 count 的增加不断平滑调光
            break;
            
        default:
            break;
    }
}

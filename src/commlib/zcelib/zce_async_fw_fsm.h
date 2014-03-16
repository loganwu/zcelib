

#ifndef ZCE_LIB_ASYNC_FRAMEWORK_FSM_
#define ZCE_LIB_ASYNC_FRAMEWORK_FSM_


#include "zce_async_fw_base.h"

//====================================================================================

/*!
* @brief      状态机对象
*/
class ZCE_Async_FSM :private ZCE_Async_Object
{
    friend class ZCE_Async_FSMMgr;


public:
    /*!
    * @brief      构造函数，
    * @param      async_mgr ,协程异步管理器的指针
    */
    ZCE_Async_FSM(ZCE_Async_FSMMgr *async_mgr);
protected:
    /*!
    * @brief      析构函数
    */
    ~ZCE_Async_FSM();


    /*!
    * @brief      继承ZCE_Async_Object的函数，
    * @param[out] continue_run 返回参数，返回当前的协程是否要继续运行下去
    */
    virtual void on_run(bool &continue_run);

    /*!
    * @brief      异步对象超时处理
    * @param[in]  now_time  发生超时的时间，
    * @param[out] continue_run 异步对象是否继续运行,
    */
    virtual void on_timeout(const ZCE_Time_Value & now_time,
        bool &continue_run);

    

    void set_stage(int stage);

    int get_stage();

private:

    ///状态机的阶段
    int               fsm_stage_;
}


//====================================================================================

/*!
* @brief      状态机主控管理类
*
*/
class ZCE_Async_FSMMgr :public ZCE_Async_ObjectMgr
{
public:

    //
    ZCE_Async_FSMMgr(ZCE_Timer_Queue *timer_queue);
    virtual ~ZCE_Async_FSMMgr();

protected:


    ///默认状态机对象池子的初始化的数量,状态机不太消耗内存，可以追求更好的性能
    static const size_t FSM_POOL_INIT_SIZE = 4;
    ///默认状态机扩展的时候，扩展的异步对象的数量
    static const size_t FSM_POOL_EXTEND_SIZE = 256;

};



#endif //#ifndef ZCE_LIB_ASYNC_FRAMEWORK_FSM_


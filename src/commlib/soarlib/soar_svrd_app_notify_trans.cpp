#include "soar_predefine.h"
#include "soar_error_code.h"
#include "soar_zerg_frame_malloc.h"
#include "soar_svrd_config.h"
#include "soar_fsm_notify_trans_mgr.h"
#include "soar_fsm_notify_taskbase.h"
#include "soar_fsm_notify_transbase.h"
#include "soar_svrd_app_notify_trans.h"

Comm_SvrdApp_NotifyTrans::Comm_SvrdApp_NotifyTrans()
{

};

Comm_SvrdApp_NotifyTrans::~Comm_SvrdApp_NotifyTrans()
{

};

//增加调用register_func_cmd
int
Comm_SvrdApp_NotifyTrans::init_instance()
{
    int ret = 0;
    ret = Comm_Svrd_Appliction::init_instance();

    if (SOAR_RET::SOAR_RET_SUCC != ret)
    {
        return ret;
    }

    THREADMUTEX_APPFRAME_MALLOCOR::instance()->initialize();

    Comm_Svrd_Config *svd_config = Comm_Svrd_Config::instance();
    MT_NOTIFY_TRANS_MANGER *trans_mgr = new MT_NOTIFY_TRANS_MANGER();
    Transaction_Manager::instance(trans_mgr);
    ZCE_Time_Value enqueue_timeout;
    enqueue_timeout.sec(svd_config->framework_config_.task_info_.enqueue_timeout_sec_);
    enqueue_timeout.usec(svd_config->framework_config_.task_info_.enqueue_timeout_usec_);
    //事务管理器的初始化
    trans_mgr->initialize(
        svd_config->framework_config_.trans_info_.trans_cmd_num_,
        svd_config->framework_config_.trans_info_.trans_num_,
        self_services_id_,
        enqueue_timeout,
        ZCE_Timer_Queue::instance(),
        Zerg_MMAP_BusPipe::instance(),
        THREADMUTEX_APPFRAME_MALLOCOR::instance());

    ret = register_notifytrans_cmd();

    if (ret != SOAR_RET::SOAR_RET_SUCC)
    {
        return ret;
    }

    NotifyTrans_TaskBase *clone_task = NULL;
    size_t task_num = 0;
    size_t task_stack_size = 0;

    // task_num task_stack_size这里不再使用，原因见下段文字
    ret = register_notify_task(clone_task,
                               task_num,
                               task_stack_size);

    // 由于app配置的初始化是在框架初始化后，所以这里调用register_notify_task时，
    // app实际上还没有加载配置，考虑到task的配置属于框架更合理一些，
    // 这里将Task的配置移到了framework.xml配置中
    // 初始化DB线程，
    ret = trans_mgr->active_notify_task(
              clone_task,
              svd_config->framework_config_.task_info_.task_thread_num_,
              svd_config->framework_config_.task_info_.task_thread_stack_size_);

    if (ret != SOAR_RET::SOAR_RET_SUCC)
    {
        ZLOG_INFO("[framework] InitInstance DBSvrdTransactionManger fail.Ret = %u", ret);
        return ret;
    }

    return SOAR_RET::SOAR_RET_SUCC;
}

//运行处理,
int Comm_SvrdApp_NotifyTrans::run_instance()
{
    // fix me add log
    ZLOG_INFO("[framework] app %s class [%s] run_instance start.",
              get_app_basename(),
              typeid(*this).name());

    size_t all_proc_frame = 0 , all_gen_trans = 0;
    size_t prcframe_queue = 0 , gentrans_queue = 0, num_timer_expire = 0, num_io_event = 0;
    size_t idle = 0;

    MT_NOTIFY_TRANS_MANGER *notify_trans_mgr = static_cast<MT_NOTIFY_TRANS_MANGER *>(Transaction_Manager::instance());
    ZCE_Time_Value select_interval(0, 0);

    ZCE_Timer_Queue *time_queue = ZCE_Timer_Queue::instance();
    ZCE_Reactor *reactor = ZCE_Reactor::instance();

    for (; app_run_;)
    {
        // 检查是否需要重新加载配置
        if (app_reload_)
        {
            // 重新加载配置
            reload_config();
        }

        //从PIPE处理收到的命令
        notify_trans_mgr->process_pipe_frame(all_proc_frame, all_gen_trans);
        //从RECV QUEUE处理命令
        notify_trans_mgr->process_recvqueue_frame(prcframe_queue, gentrans_queue);
        all_proc_frame += prcframe_queue;
        all_gen_trans += gentrans_queue;

        //超时
        num_timer_expire = time_queue->expire();

        // 处理网络包
        reactor->handle_events(&select_interval, &num_io_event);

        if ((all_proc_frame + num_timer_expire + num_io_event) <= 0)
        {
            ++idle;
        }
        else
        {
            idle = 0;
        }

        //如果忙，继续干活
        if (idle < LIGHT_IDLE_SELECT_INTERVAL)
        {
            select_interval.usec(0);
            continue;
        }
        //如果空闲很多,休息一下,如果你比较空闲，在这儿SELECT相当于Sleep，
        else if (idle >= HEAVY_IDLE_SLEEP_INTERVAL)
        {
            select_interval.usec(HEAVY_IDLE_INTERVAL_MICROSECOND );
        }
        //else 相当于 else if (idle >= LIGHT_IDLE_SELECT_INTERVAL)
        else
        {
            select_interval.usec(LIGHT_IDLE_INTERVAL_MICROSECOND );
        }
    }

    ZLOG_INFO("[framework] DBSvrdAppliction Run end.");
    return SOAR_RET::SOAR_RET_SUCC;
}

//退出处理
int Comm_SvrdApp_NotifyTrans::exit_instance()
{
    //通知所有的线程退出
    MT_NOTIFY_TRANS_MANGER *notify_trans_mgr = static_cast<MT_NOTIFY_TRANS_MANGER *>(Transaction_Manager::instance());
    notify_trans_mgr->stop_notify_task();

    int ret = 0;
    Transaction_Manager::clean_instance();

    //等待所有的Join的线程退出
    //ACE_Thread_Manager::instance()->wait();

    ret = Comm_Svrd_Appliction::exit_instance();

    if ( SOAR_RET::SOAR_RET_SUCC != ret )
    {
        return ret;
    }

    return SOAR_RET::SOAR_RET_SUCC;
}

